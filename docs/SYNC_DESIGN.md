# SYNC_DESIGN — FTP 同步系统设计文档

## 1. 设计目标

- 桌面端（Windows）与 NAS 之间双向音乐数据同步
- 增量同步策略（只传输差异部分，不传输全量）
- 支持取消操作、错误恢复、进度反馈

## 2. 整体架构

```
SyncCoordinator（协调层，管理 ISyncable 注册与 JSON merge）
    ↓
SyncManager（管理层，构建本地 Manifest）
    ↓
SyncWorker（执行层，后台线程运行多阶段流水线）
    ↓
FtpSession（FTP 连接管理，RAII 管理 CURL 句柄）
    ↓
FtpThreadPool（自建并发传输线程池，共享 DirExistsCache）
```

- `ISyncable` 接口：统一 playlist / metadata_overrides / resolved_duplicates / deleted_entries 四类数据的 `exportSyncData()` / `mergeSyncData()` / `importSyncData()` 行为
- 同步生命周期控制由 `AppContext::startSync()` 和 `AppContext::cancelSync()` 负责

## 3. Manifest 差异对比算法

### 数据结构

`ManifestFileEntry` 包含三项元数据：`relativePath`（相对路径）+ `size`（文件大小）+ `modified`（Unix 时间戳）。

`SyncManifest` 以 SHA1 哈希作为文件匹配 key（`QMap<QString, ManifestFileEntry>`），但**SHA1 仅用于匹配本地与远程的同名文件，不参与内容差异判断**。实际差异对比使用 `size` + `modified` 二元组。

### 对比规则

| 场景 | 操作 |
|------|------|
| 本地有，远程无 | 上传 |
| 远程有，本地无 | 下载 |
| 两边都有，修改时间不同 | 以修改时间较新的为准（Last-Write-Wins） |
| 两边都有，修改时间一致 | 跳过（无需同步） |

### 封面对比

封面对比使用 MD5 作为 key，采用补缺策略——本地有远端无则上传，远端有本地无则下载，两端都有则不操作（不比较时间戳）。

## 4. 同步流水线

```
[主线程] Pre 阶段：构建 Manifest → 收集删除列表 → 导出 sync_data.json
    ↓
[后台线程 SyncWorker]：
  ① Phase 1-2：连接 NAS（FtpSession 创建 CURL 句柄；Phase 2「本地数据校验」已合并至 Phase 1）
  ② Phase 3：获取远程清单（下载 manifest.json + sync_data.json）
  ③ Phase 4：对比差异（SyncWorker::compareManifests，纯函数）
  ④ Phase 5：并发传输文件（FtpThreadPool 多 Worker 上传/下载）
  ⑤ Phase 6：同步数据文件（合并并上传 sync_data.json）
  ⑥ Phase 7：上传本地 Manifest（合并本地+远程后写回）
    ↓
[主线程] Post 阶段：导入下载文件 → SyncCoordinator::applyAll → 刷新 UI
```

## 5. FtpThreadPool 设计

### 为什么不直接用 QThreadPool

QThreadPool 适用于无状态并发任务（如为每个任务分配独立线程）。FTP 同步场景需要：

- **DirExistsCache 共享**：多个 Worker 上传到同一目录时，共享远端目录存在检查结果，避免冗余的 FTP `CWD` 查询
- **按文件粒度任务分配**：每个文件一个任务，支持失败重试
- **单 Worker 错误隔离**：某 Worker 断连不影响其他 Worker

### 实现细节

- **每个 Worker**：独立 CURL 句柄 + 独立 FTP 连接（`FtpSession` 私有实例）
- **DirExistsCache**：基于 `std::unordered_set<QString>` + `std::shared_mutex` 实现，读用 `shared_lock`，写用 `unique_lock`；支持 `insertWithParents()` 递归缓存父目录
- **协作式取消**：`std::atomic<bool> cancelled` 唤醒所有 Worker 优雅终止
- **默认并发数**：4 个 Worker 线程

## 6. 四合一同步数据模型

```
playlists（歌单）
  + metadata_overrides（用户自定义元数据覆写）
  + resolved_duplicates（去重标记）
  + deleted_entries（删除日志）
  = 单一 JSON 文件（sync_data.json）
```

- **导出**：`SyncCoordinator::exportAll()` 遍历所有 ISyncable 调用 `exportSyncData()`
- **合并策略**：Last-Write-Wins（最后写入者胜出），`SyncCoordinator::merge()` 对比本地与远程 JSON，逐字段按时间戳合并
- **应用**：`SyncCoordinator::applyAll()` 遍历所有 ISyncable 调用 `importSyncData()`
- **4 个 ISyncable 注册**：PlaylistManager / MetadataOverrideManager / DedupService / DeletedEntriesSyncable

## 7. NAS 安全机制

- **逻辑删除**：本地 `music_files` 表设 `is_deleted` 字段标记删除，非物理 DELETE
- **删除日志追踪**：`DeletedEntriesSyncable` 在 `sync_data.json` 中维护 `deleted_entries` 数组，记录删除条目的 hash + 时间戳
- **远端回收站**：原设计使用 FTP `RNFR` / `RNTO` 命令将远端文件移入回收站目录，**当前版本已注释禁用**（`sync_worker.cpp:385`），远端删除同步暂不处理

## 8. libcurl 集成

- 源码静态编译，CMake 配置通过 `CURL_DISABLE_HTTP / _LDAP / _TELNET / _DICT / _FILE / _TFTP / _RTSP / _POP3 / _IMAP / _SMTP / _GOPHER / _MQTT / _IPFS / _SMB / _WEBSOCKETS` 禁用全部非常用协议，**仅保留 FTP**
- OpenSSL 平台差异化配置：Windows 使用 MSYS2 mingw64，Android 使用预编译 arm64
- FTPS 隐式/显式加密支持（`FtpSession` 通过 `CURLOPT_USE_SSL` 配置）
- `FtpSession` 使用 RAII 模式管理 CURL 句柄：构造函数延迟初始化 → `createHandle()` 懒创建 → 析构函数 `destroyHandle()` 自动释放

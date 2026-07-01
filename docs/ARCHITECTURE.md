# ARCHITECTURE — LeticiaMusic 架构设计文档

## 1. 架构概览

```
 QML View Layer（win/Main.qml + android/Main.qml）
         ↕ contextProperty + 信号槽
 C++ ViewModel Layer（ViewModelManager + 各 ViewModel）
         ↕
 C++ Service Layer（ServiceLocator → ScanService / DedupService / PlaybackService / InitOrchestrator）
         ↕
 C++ Business Layer（AppContext 单例 → PlayerController / PlaylistManager / MusicScanner / SyncCoordinator）
         ↕
 Data Layer（SqliteDatabaseService → SQLite 5 张业务表）
```

- **View 层**：纯 QML 声明式 UI，Windows 桌面端与 Android 移动端两套独立界面，共用 `common/` 共享控件
- **ViewModel 层**：C++ 实现的 QAbstractListModel 子类，通过 `rootContext()->setContextProperty()` 暴露给 QML，承担数据格式化与列表逻辑
- **Service 层**：ServiceLocator 统一管理服务实例，封装扫描、去重、播放控制、启动编排等可复用业务能力
- **Business 层**：AppContext 单例作为应用中枢，持有 PlayerController / PlaylistManager / MusicScanner / SyncCoordinator 等核心模块
- **Data 层**：SqliteDatabaseService 封装 Qt6::Sql，管理 SQLite 数据库的读写、迁移与事务

## 2. 模块职责表

| 模块 | 路径 | 职责 |
|------|------|------|
| **Application** | `src/core/application/` | AppContext 单例 + PlayerController 播放控制 + 三阶段启动编排 |
| **Database** | `src/core/database/sqlite/` | SqliteDatabaseService，管理 5 张业务表，逻辑删除策略，WAL 模式 |
| **Design** | `src/core/design/` | Design Token 体系（ColorTokens / SpacingTokens / ResponsiveTokens / FontTokens） |
| **Filesystem** | `src/core/filesystem/` | 音乐文件扫描（MusicScanner + MusicFileManager）+ 元数据覆写 + 封面管理 |
| **Playlist** | `src/core/playlist/` | PlaylistManager + FavoriteManager，UUID 跨设备标识 |
| **Services** | `src/core/services/` | ServiceLocator 服务定位器 + ScanService / DedupService / PlaybackService / InitOrchestrator |
| **Sync** | `src/core/sync/` | FTP 同步系统（SyncCoordinator → SyncManager → SyncWorker → Manifest → FtpSession → FtpThreadPool） |
| **ViewModels** | `src/core/viewmodels/` | ViewModelManager 模板工厂 + LocalMusicViewModel / PlaylistViewModel 等 |
| **Threading** | `src/core/threading/` | ThreadPoolManager（IO_POOL / CPU_POOL / DB_POOL 三线程池） |
| **Utils** | `src/core/utils/` | Logger（线程安全日志）+ PinyinUtils（拼音排序）+ DedupEngine（去重）+ SingletonHolder + TagLibUtils |
| **Platform** | `src/core/platform/` | AndroidMediaScanner（封装 Android MediaStore API） |
| **Interfaces** | `src/core/interfaces/` | IDatabaseService + IMusicEntityViewModel（接口抽象层） |

## 3. 关键设计决策

### MVVM 架构选择
**做法**：QML 作为纯声明式 View 层，C++ ViewModel 通过 `Q_PROPERTY` + 信号槽驱动 UI 刷新，Service 层封装独立业务逻辑。

**原因**：QML 声明式语法天然适合数据绑定；ViewModel 层纯 C++ 可脱离 UI 单独测试；Service 层可被多个 ViewModel 复用，避免代码重复。

### SingletonHolder\<T\> 模板单例（CRTP）
**做法**：模板基类 `SingletonHolder<T>` 统一管理 6 个核心单例（AppContext / PlayerController / Logger / ServiceLocator / FavoriteManager / ThreadPoolManager），禁用拷贝/移动构造。

**原因**：统一生命周期控制，避免静态初始化顺序问题（Static Initialization Order Fiasco），确保单例在各线程安全访问。

### 逻辑删除 vs 物理删除
**做法**：`music_files` 表设 `is_deleted` 字段，删除操作仅标记而非物理删除；同步系统通过 `DeletedEntriesSyncable`（实现 ISyncable 接口）追踪删除条目。

**原因**：物理 DELETE 会触发 SQLite 锁表，影响并发读写性能；逻辑删除支持"撤销删除"操作；`deleted_entries` 日志使跨设备同步系统能够追踪删除操作。

### IO / CPU / DB 三线程池分离
**做法**：ThreadPoolManager 定义 `IO_POOL` / `CPU_POOL` / `DB_POOL` 三个独立线程池枚举，分别承载文件 I/O、哈希计算与元数据解析、数据库写入任务。

**原因**：IO 线程读文件、CPU 线程算 SHA1/解析 TagLib 元数据、DB 线程写 SQLite，三者类型分离互不阻塞，避免 IO 等待拖慢 CPU 密集型任务。

### libcurl 裁剪编译
**做法**：CMakeLists.txt 中通过 `CURL_DISABLE_HTTP / _LDAP / _TELNET / _DICT / _FILE / _TFTP / _RTSP / _POP3 / _IMAP / _SMTP / _GOPHER / _MQTT / _IPFS / _SMB / _WEBSOCKETS` 禁用全部非常用协议，仅保留 FTP。

**原因**：项目仅需 FTP 协议用于 NAS 同步，裁剪后静态链接二进制体积减少约 60%。

### 数据库自适应批量事务
**做法**：ScanService 中根据待写入数据量动态调整事务提交粒度——≤50 条全量单次提交、≤500 条每 100 条提交、>500 条每 200 条提交。

**原因**：少量数据时事务开销可忽略，单次提交最快；大量数据时控制事务大小，避免锁表过久影响并发查询。

### 跨平台策略
**做法**：`#ifdef Q_OS_WIN` / `Q_OS_ANDROID` 条件编译隔离平台相关代码（52 处使用，分布于 11 个文件），C++ 核心层 100% 跨平台。

**原因**：一套 C++ 代码库同时产出 Windows 桌面版和 Android 移动版，避免维护两套独立代码库。

### 四合一同步数据
**做法**：playlists + metadata_overrides + resolved_duplicates + deleted_entries 四类数据合并为单一 JSON 文件（`sync_data.json`），通过 SyncCoordinator 统一导出/合并/导入。

**原因**：单文件减少 FTP 传输次数；四类数据共享合并策略（Last-Write-Wins）；ISyncable 接口统一四者的导出/导入行为。

## 4. 数据流（两条核心链路）

### 扫描链路
```
用户点击扫描
    → ScanService 触发
    → MusicScanner 三级流水线
        ① 目录遍历（IO_POOL）
        ② SHA1 哈希计算（CPU_POOL）
        ③ TagLib 元数据提取（CPU_POOL）
    → SqliteDatabaseService 批量事务写入
    → 信号通知 QML 列表刷新
```

### 播放链路
```
用户点击歌曲
    → QML 发射信号
    → PlayerController 设置 QMediaPlayer 源
    → QAudioOutput 输出音频
    → Q_PROPERTY 更新（currentPosition / duration / playbackState）
    → QML 自动刷新 NowPlayingInfo / ProgressSlider
```

## 5. 跨平台界面策略

- QML 界面分 `win/`（桌面端）和 `android/`（移动端）两套独立界面
- 共用 `common/control/`（播放控制栏 / 进度条 / 播放模式选择器 / 正在播放信息）和 `common/icon/`（11 个图标组件）
- C++ 核心代码 100% 跨平台，不依赖任何平台 API
- `main.cpp` 中根据宏选择 QML 入口：`Q_OS_WIN → win/Main.qml` / `Q_OS_ANDROID → android/Main.qml`

## 6. 技术栈一览

| 分类 | 技术 |
|------|------|
| 语言 | C++20 |
| 框架 | Qt 6.7.2（Core / Quick / Gui / Multimedia / Sql / Network） |
| UI | QML（Windows + Android 双端独立界面） |
| 数据库 | SQLite（Qt6::Sql，WAL 模式，自适应批量事务） |
| 音频元数据 | TagLib v2.3.0（源码集成） |
| 网络传输 | libcurl 8.21.1（源码裁剪，仅 FTP 协议） |
| 加密 | OpenSSL（平台差异化配置） |
| 构建 | CMake 3.16+ + Ninja |
| 测试 | Qt Test + CTest |
| 脚本 | Python 3.11（构建 / 部署 / 测试自动化） |

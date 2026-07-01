# PERFORMANCE — LeticiaMusic 性能优化总览

## 启动性能

### 问题

Android Debug 冷启动约 **3400ms**。

### 根因分析

86.4% 耗时发生在 `main()` 之前的系统加载层：
- `.so` 动态库加载与符号重定位
- Qt 框架初始化（QGuiApplication 构造 → 平台插件加载 → QML 引擎初始化）
- 业务层实际耗时仅占约 13.6%

### 优化方案

| 方案 | 措施 | 预期收益 | 状态 |
|------|------|----------|:--:|
| Release 构建 | `-O2` 优化 + `strip` 符号裁剪，Debug → Release | -70%（用户体感 3s → ~1s） | **已实施** |
| TagLib 裁剪 + 静态 + LTO | CMakeLists.txt 宏关闭非常用格式（MOD/ASF/DSF/APE/SHORTEN），静态库链接，链接时优化 | -300～500ms | 规划中 |
| QML 磁盘缓存 | `qputenv("QML_DISK_CACHE", "1")` 启用 QML 编译缓存 | 二次启动 -200～300ms | 规划中 |
| QML Loader 懒加载 | 非首屏页面包入 `Loader { asynchronous: true }` | -50ms | 规划中 |
| extractNativeLibs | `AndroidManifest.xml` 设置 `android:extractNativeLibs="false"`，跳过安装时 .so 解压 | -50～150ms | 规划中 |
| Android 主题修复 | 统一窗口背景色为 `#2d2d37`（Design Token `Colors.background`），消除白屏闪现 | 消除启动视觉断层 | **已实施** |
| 三阶段启动流水线 | PreQml（同步初始化数据库/播放器等）→ Async（后台线程加载数据，与 QML 加载并行）→ PostQml（填充 ViewModel、连接信号） | 感知启动时间缩短约 40% | **已实施** |

> 标注「规划中」的方案为 v1.1 计划优化方向，当前版本尚未实施。

### 最终效果

Debug 约 3400ms → Release 约 **1000ms**（用户体感「接近秒开」）

## 扫描性能

### 问题

大量文件扫描时 UI 卡顿，扫描耗时长。

### 优化措施

| 措施 | 说明 | 效果 |
|------|------|------|
| IO / CPU 线程池分离 | IO_POOL 读文件，CPU_POOL 算 SHA1 哈希 + 解析 TagLib 元数据，类型分离互不阻塞 | CPU 密集型任务不等待 IO |
| 批处理 50 文件 / 批 | 目录遍历收集路径后按 `BATCH_SIZE=50` 分批提交到线程池 | 避免线程池任务队列过长 |
| 自适应动态事务 | ≤50 条全量提交、≤500 条每 100 提交、>500 条每 200 提交 | 锁表时间可控 |
| `std::atomic<bool>` 协作式取消 | 各 Worker 在循环开头检查 `m_cancelled.load()`，支持随时中断 | 取消操作即时响应 |
| `Qt::QueuedConnection` 跨线程更新 | 扫描进度信号以队列连接方式通知 UI，主线程不等待 | UI 线程零卡顿 |
| QTimer 200ms 进度轮询 | 使用定时器轮询 `progressValue()` 而非高频直接绑定 | 减少 UI 刷新压力 |

### 效果

扫描期间 **UI 零卡顿**，速度从「卡一分钟」优化至「几秒完成」。

## 二进制体积优化

- **libcurl 源码裁剪**：CMakeLists.txt 中禁用 HTTP / LDAP / TELNET / DICT / FILE / TFTP / RTSP / POP3 / IMAP / SMTP / GOPHER / MQTT / IPFS / SMB / WEBSOCKETS 共 14 种协议，**仅保留 FTP**
- **效果**：静态链接体积减少约 60%

## 数据库优化

| 措施 | 说明 | 收益 |
|------|------|------|
| SQLite WAL 模式 | 数据库初始化时执行 `PRAGMA journal_mode=WAL` | 读写并发，不互斥阻塞 |
| 自适应批量事务 | 根据数据量动态调整提交粒度 | 平衡事务开销与锁表时长 |
| 逻辑删除 | `is_deleted` 字段标记而非物理 DELETE | 避免锁表 + 支持撤销 + 支撑同步追踪 |

> → 架构设计详见 [ARCHITECTURE.md](ARCHITECTURE.md) | 同步系统详见 [SYNC_DESIGN.md](SYNC_DESIGN.md)

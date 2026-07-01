# Changelog

## v1.0.0 (2026-06-27)

### Added

- 跨平台音乐播放器（Windows 桌面 + Android 移动端）
- MVVM 四层架构（QML View ↔ C++ ViewModel ↔ Service ↔ Model）
- Design Token 设计系统（ColorTokens / SpacingTokens / ResponsiveTokens / FontTokens）
- FTP 增量同步系统（Manifest 差异对比 + FtpThreadPool 并发传输 + 多阶段流水线）
- 三级流水线音乐扫描器（目录遍历 → SHA1 哈希 → TagLib 元数据提取，支持 8 种格式）
- 智能去重引擎（标题归一化 + 艺术家模糊匹配 + 时长容差 ±3s + 音质评分）
- 歌单管理（创建/编辑/删除）+ 收藏夹 + 封面自动提取
- 中文拼音排序 + A-Z 字母快速导航索引
- 三阶段启动优化（PreQml → Async → PostQml）
- IO_POOL / CPU_POOL / DB_POOL 三线程池架构
- 线程安全日志系统（5 级日志 / 按日轮转 / Qt 框架日志拦截）
- 自适应动态数据库事务（根据数据量调整批量提交粒度）
- 第三方库集成：TagLib v2.3.0 + libcurl 8.21.1（FTP，裁剪编译）+ OpenSSL
- Python 一键构建/部署脚本（build.py）
- Qt Test + CTest 单元测试框架

### Technical Stack

- C++20 / Qt 6.7.2 / QML / SQLite / CMake 3.16+ / Ninja
- Windows（MinGW 11.2.0）+ Android（NDK 26.1，arm64-v8a）

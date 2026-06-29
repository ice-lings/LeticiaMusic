#ifndef PLATFORMPATHHELPER_H
#define PLATFORMPATHHELPER_H

#include <QString>

/* 跨平台路径工具 — 统一 Android / PC 路径策略 */
class PlatformPathHelper
{
public:
    PlatformPathHelper() = delete;

    // 应用私有数据目录（数据库、配置、封面缓存）
    //   Android: /data/data/<包名>/files
    //   PC:      exe 所在目录
    static QString appDataDir();

    // 缓存/临时目录（系统可能自动清理）
    //   Android: /data/data/<包名>/cache
    //   PC:      exe 所在目录/cache
    static QString cacheDir();

    // 用户可见文档目录（日志文件等用户可能需要查看的文件）
    //   Android: /storage/emulated/0/Documents/LeticiaMusic
    //   PC:      exe 所在目录
    static QString documentDir();

    // 日志文件目录（Android 使用外部可访问路径，用户可通过文件管理器查看）
    //   Android: /storage/emulated/0/Android/data/<pkg>/files/logs
    //   PC:      exe 所在目录/logs
    static QString logDir();

    // 下载目录（从 NAS 同步的音乐文件存放处）
    //   Android: /storage/emulated/0/Android/data/<pkg>/files/Music
    //   PC:      exe 所在目录/Music (可通过 setDownloadPath 自定义)
    static QString downloadDir();

    // Windows: 设置自定义下载路径（为空则用默认值）
    static void setDownloadPath(const QString& path);

    // 导出目录（用户可访问的外部存储，无需权限）
    //   Android: /storage/emulated/0/Android/data/<pkg>/files/export
    //   PC:      exe 所在目录/export
    static QString exportDir();

    // 确保目录存在（必要时创建）
    static void ensureDir(const QString& path);

    // 迁移文件：从旧路径移到新路径（源不存在时静默跳过）
    static void migrateFile(const QString& oldPath, const QString& newPath);

    // 迁移整个目录的内容（文件级别，非递归子目录）
    static void migrateDirContents(const QString& oldDir, const QString& newDir);
};

#endif // PLATFORMPATHHELPER_H

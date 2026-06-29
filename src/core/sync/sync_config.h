#ifndef SYNC_CONFIG_H
#define SYNC_CONFIG_H

#include <QString>
#include <QList>

// 删除日志条目 (对接同步)
struct DeletedFileLogEntry {
    QString fileHash;
    qint64 deletedAt = 0;       // Unix 时间戳
    QString sourceDir;          // 来源扫描目录
    QString status = "pending"; // "pending" | "confirmed" | "skipped"
};

// 同步配置
struct SyncConfig {
    bool enabled = false;

    // NAS 连接
    QString nasHost;
    int nasPort = 21;
    QString nasUser = "admin";
    QString nasPassword;        // 加密存储
    QString nasSyncRoot = "MUSIC/LeticiaMusic";

    // 本机标识
    QString deviceId;           // UUID (首次自动生成)
    qint64 lastSyncTime = 0;

    // 删除日志
    QList<DeletedFileLogEntry> deletedFilesLog;
};

// 音乐文件路径: music/{原始文件名}.{ext}
// 冲突时追加 hash4: music/{文件名}_{hash4}.{ext}
inline QString makeMusicPath(const QString& fileName, const QString& hash4 = QString())
{
    if (hash4.isEmpty())
        return QString("music/%1").arg(fileName);
    // 在扩展名前插入 hash4
    int dot = fileName.lastIndexOf('.');
    if (dot > 0)
        return QString("music/%1_%2.%3").arg(fileName.left(dot), hash4, fileName.mid(dot + 1));
    return QString("music/%1_%2").arg(fileName, hash4);
}

#endif // SYNC_CONFIG_H

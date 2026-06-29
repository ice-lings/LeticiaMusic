#ifndef SYNC_MANIFEST_H
#define SYNC_MANIFEST_H

#include <QString>
#include <QList>
#include <QMap>
#include <QSet>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>

// manifest.json 中的文件条目
struct ManifestFileEntry {
    QString relativePath;   // "Jay/七里香/晴天.mp3"
    qint64 size = 0;
    qint64 modified = 0;    // Unix 时间戳

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["relative_path"] = relativePath;
        obj["size"] = size;
        obj["modified"] = modified;
        return obj;
    }

    static ManifestFileEntry fromJson(const QJsonObject& obj)
    {
        ManifestFileEntry e;
        e.relativePath = obj.value("relative_path").toString();
        e.size = static_cast<qint64>(obj.value("size").toDouble());
        e.modified = static_cast<qint64>(obj.value("modified").toDouble());
        return e;
    }
};

// manifest.json 中的封面条目
struct ManifestCoverEntry {
    qint64 size = 0;
    qint64 modified = 0;

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["size"] = size;
        obj["modified"] = modified;
        return obj;
    }

    static ManifestCoverEntry fromJson(const QJsonObject& obj)
    {
        ManifestCoverEntry e;
        e.size = static_cast<qint64>(obj.value("size").toDouble());
        e.modified = static_cast<qint64>(obj.value("modified").toDouble());
        return e;
    }
};

// manifest.json 完整结构
struct SyncManifest {
    int version = 1;
    QString deviceId;
    qint64 lastSync = 0;
    int dataVersion = 0;

    // key = file_hash (SHA1, 40 hex) 或 cover_hash (MD5, 32 hex)
    QMap<QString, ManifestFileEntry> files;
    QMap<QString, ManifestCoverEntry> covers;
    QMap<QString, QVariantMap> musicMetadata;  // file_hash → {title,artist,...,cover_hash,quality}

    bool isEmpty() const { return files.isEmpty() && covers.isEmpty(); }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["version"] = version;
        obj["device_id"] = deviceId;
        obj["last_sync"] = lastSync;
        obj["data_version"] = dataVersion;

        QJsonObject filesObj;
        for (auto it = files.constBegin(); it != files.constEnd(); ++it) {
            filesObj[it.key()] = it.value().toJson();
        }
        obj["files"] = filesObj;

        QJsonObject coversObj;
        for (auto it = covers.constBegin(); it != covers.constEnd(); ++it) {
            coversObj[it.key()] = it.value().toJson();
        }
        obj["covers"] = coversObj;

        QJsonObject metaObj;
        for (auto it = musicMetadata.constBegin(); it != musicMetadata.constEnd(); ++it) {
            metaObj[it.key()] = QJsonObject::fromVariantMap(it.value());
        }
        obj["music_metadata"] = metaObj;

        return obj;
    }

    static SyncManifest fromJson(const QJsonObject& obj)
    {
        SyncManifest m;
        m.version = obj.value("version").toInt(1);
        m.deviceId = obj.value("device_id").toString();
        m.lastSync = static_cast<qint64>(obj.value("last_sync").toDouble());
        m.dataVersion = obj.value("data_version").toInt(0);

        QJsonObject filesObj = obj.value("files").toObject();
        for (auto it = filesObj.constBegin(); it != filesObj.constEnd(); ++it) {
            m.files[it.key()] = ManifestFileEntry::fromJson(it.value().toObject());
        }

        QJsonObject coversObj = obj.value("covers").toObject();
        for (auto it = coversObj.constBegin(); it != coversObj.constEnd(); ++it) {
            m.covers[it.key()] = ManifestCoverEntry::fromJson(it.value().toObject());
        }

        QJsonObject metaObj = obj.value("music_metadata").toObject();
        for (auto it = metaObj.constBegin(); it != metaObj.constEnd(); ++it) {
            m.musicMetadata[it.key()] = it.value().toObject().toVariantMap();
        }

        return m;
    }
};

// 单次同步操作计划 (SyncManager Phase 3 输出)
struct SyncPlan {
    struct FileOp {
        QString hash;           // file_hash 或 cover_hash
        QString canonicalPath;  // NAS 规范路径
        QString localAbsPath;   // 本地绝对路径 (upload 时使用)
        qint64 size = 0;
        qint64 modified = 0;
        bool isCover = false;
    };

    QList<FileOp> filesToUpload;
    QList<FileOp> filesToDownload;
    QList<FileOp> filesToDelete;      // 从 NAS 删除
    QList<FileOp> coversToUpload;
    QList<FileOp> coversToDownload;
    int conflictsResolved = 0;
    QStringList failedFiles;    // 上传/下载失败的文件路径 (用于 UI 展示)

    int totalOps() const
    {
        return filesToUpload.size()
             + filesToDownload.size()
             + filesToDelete.size()
             + coversToUpload.size()
             + coversToDownload.size();
    }

    bool isEmpty() const
    {
        return filesToUpload.isEmpty()
            && filesToDownload.isEmpty()
            && filesToDelete.isEmpty()
            && coversToUpload.isEmpty()
            && coversToDownload.isEmpty();
    }
};

#endif // SYNC_MANIFEST_H

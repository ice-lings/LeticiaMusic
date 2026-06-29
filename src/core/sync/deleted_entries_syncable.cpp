#include "deleted_entries_syncable.h"
#include "../filesystem/musicfile/musicfilemanager.h"
#include "../utils/logger.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

DeletedEntriesSyncable::DeletedEntriesSyncable(MusicFileManager* mfm)
    : m_musicFiles(mfm)
{
}

SyncTask DeletedEntriesSyncable::exportSyncData()
{
    QJsonArray arr;
    if (m_musicFiles) {
        auto deletedList = m_musicFiles->getDeletedMusicList();
        for (const auto& var : deletedList) {
            QVariantMap v = var.toMap();
            QJsonObject obj;
            obj["hash"] = v["fileHash"].toString();
            obj["title"] = v["title"].toString();
            obj["deleted_at"] = static_cast<qint64>(v["deletedAt"].toLongLong());
            obj["device_id"] = "";
            arr.append(obj);
        }
    }

    SyncTask task;
    task.section = sectionName();
    task.payload = arr;
    task.lastModified = QDateTime::currentSecsSinceEpoch();
    return task;
}

SyncTask DeletedEntriesSyncable::mergeSyncData(const SyncTask& local, const SyncTask& remote)
{
    QMap<QString, QJsonObject> best;
    auto process = [&](const QJsonArray& arr) {
        for (const auto& v : arr) {
            QJsonObject obj = v.toObject();
            QString h = obj["hash"].toString();
            if (h.isEmpty()) continue;
            if (!best.contains(h) || static_cast<qint64>(obj["deleted_at"].toDouble()) >
                static_cast<qint64>(best[h]["deleted_at"].toDouble()))
                best[h] = obj;
        }
    };

    process(local.payload.toArray());
    process(remote.payload.toArray());

    QJsonArray merged;
    for (const auto& obj : best) merged.append(obj);

    SyncTask task;
    task.section = sectionName();
    task.payload = merged;
    task.lastModified = QDateTime::currentSecsSinceEpoch();
    return task;
}

bool DeletedEntriesSyncable::importSyncData(const SyncTask& task)
{
    if (!m_musicFiles) return false;

    QJsonArray arr = task.payload.toArray();
    for (const auto& v : arr) {
        QJsonObject obj = v.toObject();
        QString h = obj["hash"].toString();
        if (h.isEmpty()) continue;

        qint64 remoteDeletedAt = static_cast<qint64>(obj["deleted_at"].toDouble());

        QVariantMap local = m_musicFiles->getMusicFileRecord(h);
        if (local.isEmpty()) continue;

        qint64 localUpdatedAt = local["updated_at"].toLongLong();

        if (remoteDeletedAt > localUpdatedAt) {
            m_musicFiles->deleteMusicByHash(h);
        }
    }
    return true;
}

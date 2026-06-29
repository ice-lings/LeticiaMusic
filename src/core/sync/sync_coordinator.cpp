#include "sync_coordinator.h"
#include "../utils/logger.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

SyncCoordinator::SyncCoordinator(QObject* parent)
    : QObject(parent)
{
}

void SyncCoordinator::registerSyncable(ISyncable* syncable)
{
    if (syncable) {
        m_syncables[syncable->sectionName()] = syncable;
    }
}

QByteArray SyncCoordinator::exportAll()
{
    QJsonObject root;
    root["version"] = 2;
    root["last_modified"] = QDateTime::currentSecsSinceEpoch();

    for (auto it = m_syncables.constBegin(); it != m_syncables.constEnd(); ++it) {
        emit sectionProgress(it.key(), 0);
        SyncTask task = it.value()->exportSyncData();
        if (!task.isEmpty()) {
            root[it.key()] = task.payload;
        }
    }

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

QByteArray SyncCoordinator::merge(const QByteArray& localJson, const QByteArray& remoteJson)
{
    QJsonObject merged;
    merged["version"] = 2;
    merged["last_modified"] = QDateTime::currentSecsSinceEpoch();

    QJsonObject local = QJsonDocument::fromJson(localJson).object();
    QJsonObject remote = remoteJson.isEmpty() ? QJsonObject() : QJsonDocument::fromJson(remoteJson).object();

    for (auto it = m_syncables.constBegin(); it != m_syncables.constEnd(); ++it) {
        emit sectionProgress(it.key(), 1);

        SyncTask localTask;
        localTask.section = it.key();
        localTask.payload = local.value(it.key());
        localTask.lastModified = static_cast<qint64>(local.value("last_modified").toDouble());

        SyncTask remoteTask;
        remoteTask.section = it.key();
        remoteTask.payload = remote.value(it.key());
        remoteTask.lastModified = static_cast<qint64>(remote.value("last_modified").toDouble());

        SyncTask mergedTask = it.value()->mergeSyncData(localTask, remoteTask);
        if (!mergedTask.isEmpty()) {
            merged[it.key()] = mergedTask.payload;
        }
    }

    return QJsonDocument(merged).toJson(QJsonDocument::Indented);
}

void SyncCoordinator::applyAll(const QByteArray& mergedJson)
{
    if (mergedJson.isEmpty()) return;

    QJsonObject merged = QJsonDocument::fromJson(mergedJson).object();

    for (auto it = m_syncables.constBegin(); it != m_syncables.constEnd(); ++it) {
        emit sectionProgress(it.key(), 2);

        SyncTask task;
        task.section = it.key();
        task.payload = merged.value(it.key());
        task.lastModified = static_cast<qint64>(merged.value("last_modified").toDouble());

        it.value()->importSyncData(task);
    }
}

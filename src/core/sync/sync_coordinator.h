#ifndef SYNC_COORDINATOR_H
#define SYNC_COORDINATOR_H

#include <QObject>
#include <QMap>
#include <QByteArray>
#include "isyncable.h"

class SyncCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit SyncCoordinator(QObject* parent = nullptr);

    void registerSyncable(ISyncable* syncable);

    QByteArray exportAll();

    QByteArray merge(const QByteArray& localJson, const QByteArray& remoteJson);

    void applyAll(const QByteArray& mergedJson);

signals:
    void sectionProgress(const QString& section, int phase);

private:
    QMap<QString, ISyncable*> m_syncables;
};

#endif // SYNC_COORDINATOR_H

#ifndef SYNC_TASK_H
#define SYNC_TASK_H

#include <QString>
#include <QJsonValue>

struct SyncTask
{
    QString section;
    QJsonValue payload;
    qint64 lastModified = 0;

    bool isEmpty() const
    {
        return payload.isNull() || payload.isUndefined();
    }
};

#endif // SYNC_TASK_H

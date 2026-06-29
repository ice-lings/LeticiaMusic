#ifndef ISYNCABLE_H
#define ISYNCABLE_H

#include "sync_task.h"

class ISyncable
{
public:
    virtual ~ISyncable() = default;

    virtual QString sectionName() const = 0;

    virtual SyncTask exportSyncData() = 0;

    virtual SyncTask mergeSyncData(const SyncTask& local,
                                    const SyncTask& remote) = 0;

    virtual bool importSyncData(const SyncTask& task) = 0;
};

#endif // ISYNCABLE_H

#ifndef DELETED_ENTRIES_SYNCABLE_H
#define DELETED_ENTRIES_SYNCABLE_H

#include "isyncable.h"

class MusicFileManager;

class DeletedEntriesSyncable : public ISyncable
{
public:
    explicit DeletedEntriesSyncable(MusicFileManager* mfm);

    QString sectionName() const override { return "deleted_entries"; }
    SyncTask exportSyncData() override;
    SyncTask mergeSyncData(const SyncTask& local, const SyncTask& remote) override;
    bool importSyncData(const SyncTask& task) override;

private:
    MusicFileManager* m_musicFiles = nullptr;
};

#endif // DELETED_ENTRIES_SYNCABLE_H

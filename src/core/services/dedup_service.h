#ifndef DEDUP_SERVICE_H
#define DEDUP_SERVICE_H

#include <QObject>
#include <QVariantList>
#include <QStringList>
#include "../sync/isyncable.h"

class MusicFileManager;
class MusicCoverManager;
class ViewModelManager;

class DedupService : public QObject, public ISyncable
{
    Q_OBJECT
public:
    DedupService(MusicFileManager* mfm, MusicCoverManager* coverMgr,
                 ViewModelManager* vmMgr, QObject* parent = nullptr);

    Q_INVOKABLE QVariantList findDuplicateSongs();
    Q_INVOKABLE void resolveDuplicate(const QString& keepHash, const QStringList& deleteHashes);

    // ISyncable
    QString sectionName() const override { return "resolved_duplicates"; }
    SyncTask exportSyncData() override;
    SyncTask mergeSyncData(const SyncTask& local, const SyncTask& remote) override;
    bool importSyncData(const SyncTask& task) override;

signals:
    void musicDeleted(const QString& musicHash);
    void deletedMusicCountChanged();

private:
    MusicFileManager*  m_fileManager;
    MusicCoverManager* m_coverManager;
    ViewModelManager*  m_vmManager;
};
#endif

#ifndef METADATAOVERRIDEMANAGER_H
#define METADATAOVERRIDEMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QHash>
#include "../../sync/isyncable.h"

class IDatabaseService;

// TODO: Phase 2 - TagLib 写回原文件 + "写入文件"按钮，暂不实现

class MetadataOverrideManager : public QObject, public ISyncable
{
    Q_OBJECT
public:
    explicit MetadataOverrideManager(QObject* parent = nullptr,
                                     IDatabaseService* db = nullptr);

    bool initialize();

    void setMetadata(const QString& fileHash, const QVariantMap& data);
    void setCoverHash(const QString& fileHash, const QString& coverHash);

    QVariantMap getOverrides(const QString& fileHash) const;
    bool hasOverrides(const QString& fileHash) const;
    QHash<QString, QVariantMap> getAllOverrides() const;

    void removeOverride(const QString& fileHash);

    // ISyncable
    QString sectionName() const override { return "metadata_overrides"; }
    SyncTask exportSyncData() override;
    SyncTask mergeSyncData(const SyncTask& local, const SyncTask& remote) override;
    bool importSyncData(const SyncTask& task) override;

private:
    bool initializeTable();
    void loadAllOverridesToCache();

    IDatabaseService* m_db = nullptr;
    QHash<QString, QVariantMap> m_cache;

    const QString TABLE_NAME = "metadata_overrides";
};

#endif // METADATAOVERRIDEMANAGER_H

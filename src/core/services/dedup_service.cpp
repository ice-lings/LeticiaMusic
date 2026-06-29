#include "dedup_service.h"
#include "../application/appconfig.h"
#include "../filesystem/musicfile/musicfilemanager.h"
#include "../filesystem/metadata/musiccovermanager.h"
#include "../viewmodels/viewmodelmanager.h"
#include "../viewmodels/music/local_music_view_model.h"
#include "../utils/dedup_engine.h"
#include "../utils/logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QFileInfo>
#include <algorithm>

DedupService::DedupService(MusicFileManager* mfm, MusicCoverManager* coverMgr,
                           ViewModelManager* vmMgr, QObject* parent)
    : QObject(parent)
    , m_fileManager(mfm)
    , m_coverManager(coverMgr)
    , m_vmManager(vmMgr)
{
}

QVariantList DedupService::findDuplicateSongs()
{
    QVariantList result;
    if (!m_fileManager) return result;

    const auto& musicList = m_fileManager->getMusicList();
    const auto groups = DedupEngine::buildDuplicateGroups(musicList, AppConfig::instance().resolvedDuplicateHashes());

    for (const auto& g : groups) {
        QVariantMap group;
        group["groupKey"] = g.groupKey;
        group["displayTitle"] = g.displayTitle;
        group["displayArtist"] = g.displayArtist;

        QVariantList entries;
        for (const auto& e : g.entries) {
            entries.append(e);
        }

        std::sort(entries.begin(), entries.end(), [](const QVariant& a, const QVariant& b) {
            auto ma = a.toMap(), mb = b.toMap();
            QString da = ma["duration"].toString(), db = mb["duration"].toString();
            return da < db;
        });
        group["entries"] = entries;

        QString bestHash;
        int bestScore = -1;
        for (const auto& e : entries) {
            auto m = e.toMap();
            QString q = m["quality"].toString();
            int score = DedupEngine::qualityScore(q, 0, "");
            if (score > bestScore) {
                bestScore = score;
                bestHash = m["fileHash"].toString();
            }
        }
        group["recommendKeep"] = bestHash;

        result.append(group);
    }

    Log.info(QString("[去重] 发现 %1 组重复歌曲").arg(result.size()));
    return result;
}

void DedupService::resolveDuplicate(const QString& keepHash, const QStringList& deleteHashes)
{
    if (!m_fileManager) return;

    auto* localViewModel = m_vmManager
        ? m_vmManager->getViewModel<LocalMusicViewModel>("local_music") : nullptr;

    for (const auto& hash : deleteHashes) {
        if (hash == keepHash) {
            AppConfig::instance().addResolvedDuplicateHash(hash);
            continue;
        }

        MusicItem item = m_fileManager->getMusicItemByFileHash(hash);
        const QString coverHash = item.coverHash;
        const QString filePath = item.filePath;
        const QString title = item.title;

        m_fileManager->deleteMusicByHash(hash);
        Log.info(QString("[去重] 删除: %1").arg(hash));

        if (localViewModel) {
            localViewModel->removeByHash(hash);
        }

        if (!coverHash.isEmpty() && m_coverManager) {
            int remainingRefs = m_fileManager->countByCoverHash(coverHash);
            if (remainingRefs == 0) {
                Log.info(QString("[去重] 封面 %1 无其他引用，删除").arg(coverHash));
                m_coverManager->deleteCover(coverHash);
            }
        }

        AppConfig::instance().addResolvedDuplicateHash(hash);
    }

    if (!keepHash.isEmpty()) {
        AppConfig::instance().addResolvedDuplicateHash(keepHash);
    }
    AppConfig::instance().save();

    emit deletedMusicCountChanged();
    emit musicDeleted(keepHash);
}

SyncTask DedupService::exportSyncData()
{
    QJsonObject root;
    root["version"] = 1;
    root["last_modified"] = QDateTime::currentSecsSinceEpoch();

    QJsonArray hashes;
    for (const auto& h : AppConfig::instance().resolvedDuplicateHashes()) {
        hashes.append(h);
    }
    root["hashes"] = hashes;

    SyncTask task;
    task.section = sectionName();
    task.payload = root;
    task.lastModified = QDateTime::currentSecsSinceEpoch();
    return task;
}

SyncTask DedupService::mergeSyncData(const SyncTask& local, const SyncTask& remote)
{
    QJsonArray localHashes = local.payload.toObject().value("hashes").toArray();
    QJsonArray remoteHashes = remote.payload.toObject().value("hashes").toArray();

    QJsonArray merged;
    QSet<QString> seen;
    for (const auto& v : localHashes) {
        QString h = v.toString();
        if (!h.isEmpty() && !seen.contains(h)) { seen.insert(h); merged.append(h); }
    }
    for (const auto& v : remoteHashes) {
        QString h = v.toString();
        if (!h.isEmpty() && !seen.contains(h)) { seen.insert(h); merged.append(h); }
    }

    QJsonObject root;
    root["version"] = 1;
    root["last_modified"] = QDateTime::currentSecsSinceEpoch();
    root["hashes"] = merged;

    SyncTask task;
    task.section = sectionName();
    task.payload = root;
    task.lastModified = QDateTime::currentSecsSinceEpoch();
    return task;
}

bool DedupService::importSyncData(const SyncTask& task)
{
    QJsonArray hashes = task.payload.toObject().value("hashes").toArray();
    for (const auto& v : hashes) {
        QString h = v.toString();
        if (!h.isEmpty()) AppConfig::instance().addResolvedDuplicateHash(h);
    }
    return true;
}

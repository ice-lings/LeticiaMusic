#include "sync_manager.h"
#include "../filesystem/musicfile/musicfilemanager.h"
#include "../filesystem/metadata/musiccovermanager.h"
#include "../playlist/playlistmanager.h"
#include "../utils/logger.h"
#include "../utils/platform_path_helper.h"
#include "../application/appconfig.h"

#include <QFileInfo>
#include <QDir>
#include <QDateTime>

SyncManager::SyncManager(QObject* parent)
    : QObject(parent)
{
}

void SyncManager::setMusicFileManager(MusicFileManager* mgr) { m_musicFiles = mgr; }
void SyncManager::setMusicCoverManager(MusicCoverManager* mgr) { m_covers = mgr; }
void SyncManager::setPlaylistManager(PlaylistManager* mgr) { m_playlists = mgr; }

SyncManifest SyncManager::buildLocalManifest()
{
    SyncManifest manifest;
    manifest.deviceId = m_syncConfig.deviceId;
    manifest.lastSync = m_syncConfig.lastSyncTime;

    const auto& musicList = m_musicFiles->getMusicList();

    int skippedNoHash = 0;
    int skippedNotExists = 0;
    int included = 0;

    QSet<QString> usedNames;
    int total = musicList.size();
    int current = 0;

    for (const auto& item : musicList) {
        if (item.fileHash.isEmpty()) {
            skippedNoHash++;
            continue;
        }

        current++;
        emit syncProgress(current, total, item.filePath);

        QFileInfo fi(item.filePath);
        if (!fi.exists()) {
            skippedNotExists++;
            continue;
        }

        QString fileName = fi.fileName();
        if (AppConfig::instance().hasCanonicalPath(item.fileHash))
            fileName = QFileInfo(AppConfig::instance().canonicalPath(item.fileHash)).fileName();
        QString relativePath;
        if (usedNames.contains(fileName)) {
            QString hash4 = item.fileHash.left(4);
            relativePath = makeMusicPath(fileName, hash4);
        } else {
            relativePath = makeMusicPath(fileName);
        }
        usedNames.insert(fileName);

        ManifestFileEntry entry;
        entry.relativePath = relativePath;
        entry.size = fi.size();
        entry.modified = fi.lastModified().toSecsSinceEpoch();

        manifest.files[item.fileHash] = entry;
        included++;

        if (!item.coverHash.isEmpty()) {
            QString coverPath = m_covers->getCoverPath(item.coverHash);
            if (!coverPath.isEmpty()) {
                QFileInfo ci(coverPath);
                if (ci.exists()) {
                    ManifestCoverEntry ce;
                    ce.size = ci.size();
                    ce.modified = ci.lastModified().toSecsSinceEpoch();
                    manifest.covers[item.coverHash] = ce;
                }
            }
        }
    }

    auto sanitizeName = [](const QString& name) -> QString {
        QString result = name.trimmed();
        static const QChar invalidChars[] = {'/', '\\', ':', '*', '?', '"', '<', '>', '|'};
        for (QChar c : invalidChars) result.replace(c, '_');
        result.replace(QStringLiteral("  "), QStringLiteral(" "));
        return result.isEmpty() ? QStringLiteral("\xe6\x94\xb6\xe8\x97\x8f\xe6\xad\x8c\xe5\x8d\x95") : result;
    };

    if (m_playlists) {
        const auto& collections = m_playlists->getCollectionPlaylists();
        for (const auto& pl : collections) {
            QString folderPrefix = sanitizeName(pl.name);
            const auto& items = m_playlists->getPlaylistItems(pl.id);
            for (const auto& pi : items) {
                if (pi.musicHash.isEmpty()) continue;
                if (manifest.files.contains(pi.musicHash)) continue;

                MusicItem mi = m_musicFiles->loadMusicItemByHash(pi.musicHash);
                if (mi.fileHash.isEmpty()) {
                    skippedNoHash++;
                    continue;
                }

                QFileInfo fi(mi.filePath);
                if (!fi.exists()) {
                    skippedNotExists++;
                    continue;
                }

                QString path = QString("collections/%1/%2").arg(folderPrefix, fi.fileName());

                ManifestFileEntry entry;
                entry.relativePath = path;
                entry.size = fi.size();
                entry.modified = fi.lastModified().toSecsSinceEpoch();
                manifest.files[mi.fileHash] = entry;

                if (!mi.coverHash.isEmpty() && !manifest.covers.contains(mi.coverHash)) {
                    QString coverPath = m_covers->getCoverPath(mi.coverHash);
                    if (!coverPath.isEmpty()) {
                        QFileInfo ci(coverPath);
                        if (ci.exists()) {
                            ManifestCoverEntry ce;
                            ce.size = ci.size();
                            ce.modified = ci.lastModified().toSecsSinceEpoch();
                            manifest.covers[mi.coverHash] = ce;
                        }
                    }
                }
            }
        }
    }

    auto meta = m_musicFiles->getBatchMusicMetadata();
    for (auto it = meta.constBegin(); it != meta.constEnd(); ++it)
        manifest.musicMetadata[it.key()] = it.value();

    Log.info(QString("[Sync] Manifest diagnostic: DB records=%1 skipped(noHash=%2 notExist=%3) included=%4 covers=%5")
        .arg(total).arg(skippedNoHash).arg(skippedNotExists).arg(included).arg(manifest.covers.size()));

    return manifest;
}

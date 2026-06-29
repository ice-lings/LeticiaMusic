#include "appcontext.h"

#include <QTimer>
#include <QThread>
#include <QFileInfo>
#include <QUuid>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QFile>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>
#include <algorithm>

#ifdef Q_OS_ANDROID
#include <android/log.h>
#include <QJniObject>
#include <QJniEnvironment>
#include <QtCore/private/qandroidextras_p.h>
#include "../platform/android_media_scanner.h"
#endif

#include "../filesystem/musicfile/musicfilemanager.h"
#include "../filesystem/musicfile/musicscanner.h"
#include "../utils/pinyin.h"
#include "../utils/dedup_engine.h"
#include "../filesystem/metadata/musiccovermanager.h"
#include "../utils/musicinfo.h"
#include "../database/sqlite/sqlitedatabaseservice.h"

#include "../services/scan_service.h"
#include "../services/dedup_service.h"
#include "../services/playback_service.h"
#include "../sync/sync_manager.h"
#include "../sync/sync_manifest.h"
#include "../sync/sync_config.h"
#include "../sync/sync_coordinator.h"
#include "../sync/sync_worker.h"
#include "../filesystem/metadata/metadataoverridemanager.h"
#include "../utils/platform_path_helper.h"
#include "../utils/query_options.h"



#include "../viewmodels/viewmodelmanager.h"
#include "../viewmodels/music/local_music_view_model.h"
#include "../viewmodels/playlist/playlistviewmodel.h"
#include "../viewmodels/playlist/playlistdetailviewmodel.h"
#include "../viewmodels/playlist/collectionplaylistviewmodel.h"
#include "../utils/logger.h"
#include "../utils/musicinfo.h"
#include "../services/init_steps.h"

#include <QDebug>
#include <QModelIndex>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>

static qint64 s_startupEpochMs = 0;

AppContext::AppContext(QObject* parent) : QObject(parent) {
    m_initOrch = new InitOrchestrator(this);
    registerInitSteps();
}

void AppContext::registerInitSteps()
{
    auto& orch = *m_initOrch;
    orch.addStep<DatabaseInitStep>();
    orch.addStep<PlayerInitStep>();
    orch.addStep<SchemaInitStep>();
    orch.addStep<PlaylistInitStep>();
    orch.addStep<ViewModelRegisterStep>();
    orch.addStep<DataLoadInitStep>();
    orch.addStep<ViewModelPopulateStep>();
    orch.addStep<ServiceInitStep>();
    orch.addStep<SignalConnectStep>();
    orch.addStep<RestorePlaybackStep>();
}

void AppContext::recordStartupTime()
{
    s_startupEpochMs = QDateTime::currentMSecsSinceEpoch();
}

void AppContext::markFirstFrameRendered()
{
    if (s_startupEpochMs > 0) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - s_startupEpochMs;
        Log.info(QString("[Timing] First frame rendered: %1ms from main()").arg(elapsed));
    } else {
        Log.warning("[Timing] First frame rendered: startup timestamp not recorded");
    }
}

#ifdef Q_OS_ANDROID
void AppContext::moveToBackground()
{
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    Log.info(QString("[BackGesture] context valid=%1, isActivity=%2")
        .arg(activity.isValid())
        .arg(QNativeInterface::QAndroidApplication::isActivityContext()));

    if (activity.isValid()) {
        activity.callMethod<jboolean>("moveTaskToBack", "(Z)Z", true);

        QJniEnvironment env;
        if (env.checkAndClearExceptions()) {
            Log.error("[BackGesture] moveTaskToBack threw JNI exception");
        } else {
            Log.info("[BackGesture] moveTaskToBack called");
        }
    } else {
        Log.error("[BackGesture] no valid Activity context");
    }
}
#endif

PlayerController* AppContext::playerController() const noexcept {
    return &PlayerController::instance();
}
FavoriteManager* AppContext::favoriteManager() const noexcept {
    return &FavoriteManager::instance();
}
PlaylistManager* AppContext::playlistManager() const noexcept {
    return ServiceLocator::instance().get<PlaylistManager>();
}
PlaylistViewModel* AppContext::playlistViewModel() const noexcept {
    auto* vm = ServiceLocator::instance().get<ViewModelManager>();
    return vm ? vm->getViewModel<PlaylistViewModel>("playlists") : nullptr;
}
CollectionPlaylistViewModel* AppContext::collectionPlaylistViewModel() const noexcept {
    auto* vm = ServiceLocator::instance().get<ViewModelManager>();
    return vm ? vm->getViewModel<CollectionPlaylistViewModel>("collection_playlists") : nullptr;
}
PlaylistDetailViewModel* AppContext::playlistDetailViewModel() const noexcept {
    auto* vm = ServiceLocator::instance().get<ViewModelManager>();
    return vm ? vm->getViewModel<PlaylistDetailViewModel>("playlist_detail") : nullptr;
}
AppConfig* AppContext::config() const noexcept {
    return &AppConfig::instance();
}

bool AppContext::isScanning() const noexcept {
    auto* ss = ServiceLocator::instance().get<ScanService>();
    return ss ? ss->isScanning() : false;
}
int AppContext::scanPhase() const noexcept {
    auto* ss = ServiceLocator::instance().get<ScanService>();
    return ss ? ss->scanPhase() : 0;
}
int AppContext::scanCurrent() const noexcept {
    auto* ss = ServiceLocator::instance().get<ScanService>();
    return ss ? ss->scanCurrent() : 0;
}
int AppContext::scanTotal() const noexcept {
    auto* ss = ServiceLocator::instance().get<ScanService>();
    return ss ? ss->scanTotal() : 0;
}
QString AppContext::scanCurrentFile() const noexcept {
    auto* ss = ServiceLocator::instance().get<ScanService>();
    return ss ? ss->scanCurrentFile() : QString();
}
QString AppContext::scanPhaseText() const noexcept {
    auto* ss = ServiceLocator::instance().get<ScanService>();
    return ss ? ss->scanPhaseText() : QString();
}
bool AppContext::isSyncRunning() const noexcept {
    return m_syncRunning;
}
int AppContext::deletedMusicCount() const noexcept {
    auto* mfm = ServiceLocator::instance().get<MusicFileManager>();
    return mfm ? mfm->getDeletedMusicCount() : 0;
}

QObject *AppContext::getViewModel(const QString &name)
{
    if (!ServiceLocator::instance().get<ViewModelManager>()) {
        Log.error("[AppContext] ViewModelManager not initialized");
        return nullptr;
    }
    return qobject_cast<QObject*>(ServiceLocator::instance().get<ViewModelManager>()->getViewModel(name));
}

QVariantMap AppContext::getMusicItemForFile(const QString& filePath)
{
    QVariantMap result;

    if (filePath.isEmpty()) {
        return result;
    }

    MusicItem item;
    if (ServiceLocator::instance().get<MusicFileManager>()) {
        item = ServiceLocator::instance().get<MusicFileManager>()->getMusicItemByFilePath(filePath);
    }
    if (item.filePath.isEmpty() && ServiceLocator::instance().get<ViewModelManager>()) {
        if (auto* vm = ServiceLocator::instance().get<ViewModelManager>()->getViewModel<LocalMusicViewModel>("local_music")) {
            const int row = vm->indexForFilePath(filePath);
            if (row >= 0) {
                const QModelIndex mi = vm->index(row, 0);
                item.title = vm->data(mi, LocalMusicViewModel::TitleRole).toString();
                item.artist = vm->data(mi, LocalMusicViewModel::ArtistRole).toString();
                item.album = vm->data(mi, LocalMusicViewModel::AlbumRole).toString();
                item.duration = vm->data(mi, LocalMusicViewModel::DurationRole).toString();
                item.coverPath = vm->data(mi, LocalMusicViewModel::CoverArtRole).toUrl();
                item.filePath = vm->data(mi, LocalMusicViewModel::FilePathRole).toString();
            }
        }
    }
    if (item.filePath.isEmpty()) {
        return result;
    }

    if (!item.coverHash.isEmpty() && item.coverPath.isEmpty()) {
        auto* cm = ServiceLocator::instance().get<MusicCoverManager>();
        if (cm) {
            QString cp = cm->getCoverPath(item.coverHash);
            if (!cp.isEmpty()) item.coverPath = QUrl::fromLocalFile(cp);
        }
    }

    result["title"] = item.title;
    result["artist"] = item.artist;
    result["album"] = item.album;
    result["duration"] = item.duration;
    result["coverPath"] = item.coverPath.toString();
    result["filePath"] = item.filePath;

    return result;
}

QString AppContext::getCoverPathByFileHash(const QString& fileHash)
{
    if (fileHash.isEmpty() || !ServiceLocator::instance().get<MusicFileManager>()) {
        return "";
    }
    
    MusicItem item = ServiceLocator::instance().get<MusicFileManager>()->getMusicItemByFileHash(fileHash);
    if (!item.coverPath.isEmpty()) {
        return item.coverPath.toString();
    }
    
    return "";
}

void AppContext::applyMetadataOverrides(QList<MusicItem>& items)
{
    auto* om = ServiceLocator::instance().get<MetadataOverrideManager>();
    if (!om) return;

    const auto overrides = om->getAllOverrides();
    if (overrides.isEmpty()) return;

    for (auto& item : items) {
        auto it = overrides.find(item.fileHash);
        if (it == overrides.end()) continue;
        const QVariantMap& ov = it.value();
        if (ov.contains("title"))    item.title = ov["title"].toString();
        if (ov.contains("artist"))   item.artist = ov["artist"].toString();
        if (ov.contains("album"))    item.album = ov["album"].toString();
        if (ov.contains("cover_hash") && !ov["cover_hash"].isNull()) {
            item.coverHash = ov["cover_hash"].toString();
            item.coverPath = QUrl::fromLocalFile(
                ServiceLocator::instance().get<MusicCoverManager>()->getCoverPath(item.coverHash));
        }
    }
}

void AppContext::importDownloadedFiles(const QList<QPair<QString, QString>>& files,
                                       const SyncManifest& manifest)
{
    auto* mfm = ServiceLocator::instance().get<MusicFileManager>();
    auto* cm  = ServiceLocator::instance().get<MusicCoverManager>();
    if (!mfm || !cm || files.isEmpty()) return;

    QSet<QString> existing = mfm->getAllMusicIds();
    QList<MusicItem> imported;

    for (const auto& pair : files) {
        const QString& hash = pair.first;
        const QString& localPath = pair.second;

        if (existing.contains(hash)) continue;
        if (!QFile::exists(localPath)) continue;

        auto it = manifest.musicMetadata.find(hash);
        if (it == manifest.musicMetadata.end()) continue;
        const QVariantMap& meta = it.value();
        if (meta.isEmpty()) continue;

        MusicItem mi;
        mi.fileHash = hash;
        mi.filePath = localPath;
        mi.title = meta.value("title").toString();
        mi.artist = meta.value("artist").toString();
        if (mi.artist.isEmpty()) mi.artist = QString::fromUtf8("\xe4\xbd\x9a\xe5\x90\x8d");
        mi.album = meta.value("album").toString();
        mi.duration = formatDuration(meta.value("length_in_seconds").toInt());
        mi.sectionKey = PinyinUtils::sectionKey(
            mi.title.isEmpty() ? QFileInfo(localPath).completeBaseName() : mi.title);

        QString covHash = meta.value("cover_hash").toString();
        if (!covHash.isEmpty()) {
            QString covPath = cm->registerCoverIfExists(covHash);
            if (!covPath.isEmpty()) {
                mi.coverHash = covHash;
                mi.coverPath = QUrl::fromLocalFile(covPath);
            }
        }

        MusicInfo info;
        info.musicID = hash;
        info.musicFilePath = localPath;
        info.coverHash = covHash;
        info.metadata.title = mi.title;
        info.metadata.artist = mi.artist;
        info.metadata.album = mi.album;
        info.metadata.lengthInSeconds = meta.value("length_in_seconds").toInt();
        info.metadata.bitrate = meta.value("bitrate").toInt();
        info.metadata.sampleRate = meta.value("sample_rate").toInt();
        info.metadata.channels = meta.value("channels").toInt();
        info.metadata.audioFormat = meta.value("audio_format").toString();

        mfm->trySaveMusicFileInfoToDB(info, mi);
        imported.append(mi);
    }

    applyMetadataOverrides(imported);

    if (!imported.isEmpty()) {
        if (auto* lvm = ServiceLocator::instance().get<ViewModelManager>()
                ->getViewModel<LocalMusicViewModel>("local_music")) {
            lvm->onMetaDataReady(imported);
        }
    }
}

void AppContext::ensurePlaylistFilesExist(const QByteArray& mergedSyncData,
                                          const SyncManifest& manifest)
{
    QJsonObject root = QJsonDocument::fromJson(mergedSyncData).object();
    QJsonObject playlistsRoot = root.value("playlists").toObject();
    QJsonArray playlistsArr = playlistsRoot.value("playlists").toArray();

    QSet<QString> allHashes;
    for (const auto& plVal : playlistsArr) {
        QJsonArray items = plVal.toObject().value("items").toArray();
        for (const auto& itemVal : items)
            allHashes.insert(itemVal.toObject().value("music_hash").toString());
    }

    allHashes.remove(QString());

    auto* mfm = ServiceLocator::instance().get<MusicFileManager>();
    auto* db  = ServiceLocator::instance().get<SqliteDatabaseService>();
    if (!mfm || !db || allHashes.isEmpty()) return;

    QSet<QString> existing = mfm->getAllMusicIds();

    int inserted = 0;
    for (const QString& hash : allHashes) {
        if (existing.contains(hash)) continue;

        QVariantMap row;
        row["file_hash"] = hash;
        row["file_path"] = "placeholder/" + hash;
        row["file_size"] = 1;
        row["last_modified"] = 1;
        row["metadata_extracted"] = 0;
        row["cover_extracted"] = 0;

        auto it = manifest.musicMetadata.find(hash);
        if (it != manifest.musicMetadata.end()) {
            const QVariantMap& meta = it.value();
            row["title"] = meta.value("title").toString();
            row["artist"] = meta.value("artist").toString();
            if (row["artist"].toString().isEmpty())
                row["artist"] = QString::fromUtf8("\xe4\xbd\x9a\xe5\x90\x8d");
            row["album"] = meta.value("album");
            row["length_in_seconds"] = meta.value("length_in_seconds");
            row["bitrate"] = meta.value("bitrate");
            row["sample_rate"] = meta.value("sample_rate");
            row["channels"] = meta.value("channels");
            row["audio_format"] = meta.value("audio_format");
            row["cover_hash"] = meta.value("cover_hash");
            row["quality"] = meta.value("quality");
        } else {
            row["title"] = hash.left(12);
            row["artist"] = QString::fromUtf8("\xe4\xbd\x9a\xe5\x90\x8d");
        }

        db->insertIgnore("music_files", row);
        inserted++;
    }

    Log.info(QString("[Sync] ensurePlaylistFilesExist: checked=%1 inserted=%2")
        .arg(allHashes.size()).arg(inserted));
}

void AppContext::updateCoverPaths(QList<MusicItem>& musicList)
{
    for (auto& item : musicList) {
        const QString hash = !item.coverHash.isEmpty() ? item.coverHash : item.coverPath.fileName();
        if (!hash.isEmpty()) {
            item.coverPath = QUrl::fromLocalFile(
                ServiceLocator::instance().get<MusicCoverManager>()->getCoverPath(hash));
        }
    }
}

// ===== Settings accessors =====
QStringList AppContext::getMusicScanDirectories() const {
    return AppConfig::instance().musicScanDirectories();
}

void AppContext::setMusicScanDirectories(const QStringList& paths) {
    AppConfig::instance().setMusicScanDirectories(paths);
}

void AppContext::addMusicScanDirectory(const QString& path) {
    AppConfig::instance().addMusicScanDirectory(path);
}

void AppContext::removeMusicScanDirectory(const QString& path) {
    AppConfig::instance().removeMusicScanDirectory(path);
}

int AppContext::getMinFileSizeKB() const { return AppConfig::instance().minFileSizeKB(); }
void AppContext::setMinFileSizeKB(int kb) { AppConfig::instance().setMinFileSizeKB(kb); }
int AppContext::getMinDurationSec() const { return AppConfig::instance().minDurationSec(); }
void AppContext::setMinDurationSec(int sec) { AppConfig::instance().setMinDurationSec(sec); }

QVariantList AppContext::getScanFolders() const {
    return ServiceLocator::instance().get<ScanService>()->getScanFolders();
}

void AppContext::setScanFolderEnabled(const QString& path, bool enabled) {
    ServiceLocator::instance().get<ScanService>()->setScanFolderEnabled(path, enabled);
}

void AppContext::clearScanFolders() {
    ServiceLocator::instance().get<ScanService>()->clearScanFolders();
}

void AppContext::rebuildScanFoldersFromLibrary() {
    ServiceLocator::instance().get<ScanService>()->rebuildScanFoldersFromLibrary();
}

void AppContext::scanSingleDirectory(const QString& path) {
    ServiceLocator::instance().get<ScanService>()->scanSingleDirectory(path);
}

QString AppContext::getDownloadPath() const {
    return AppConfig::instance().getDownloadPath();
}

#ifndef Q_OS_ANDROID
void AppContext::setDownloadPath(const QString& path) {
    AppConfig::instance().setDownloadPath(path);
}
#endif

QString AppContext::getDefaultDownloadPath() const {
    return AppConfig::instance().getDefaultDownloadPath();
}

QString AppContext::getDatabasePath() const {
    return AppConfig::instance().databasePath();
}

void AppContext::setDatabasePath(const QString& path) {
    AppConfig::instance().setDatabasePath(path);
}

bool AppContext::getAutoScanMusicOnStartup() const {
    return AppConfig::instance().autoScanMusicOnStartup();
}

void AppContext::setAutoScanMusicOnStartup(bool enabled) {
    AppConfig::instance().setAutoScanMusicOnStartup(enabled);
}

bool AppContext::getResumePlaybackOnStartup() const {
    return AppConfig::instance().resumePlaybackOnStartup();
}

void AppContext::setResumePlaybackOnStartup(bool enabled) {
    AppConfig::instance().setResumePlaybackOnStartup(enabled);
}

int AppContext::getLastVolume() const {
    return AppConfig::instance().lastVolume();
}

void AppContext::setLastVolume(int volume) {
    AppConfig::instance().setLastVolume(volume);
}

QString AppContext::getAppVersion() const {
    return AppConfig::instance().getAppVersion();
}

// ===== Scan =====
void AppContext::scanMusicNow() {
    if (m_syncRunning) { Log.info("Sync in progress, scan blocked"); return; }
    ServiceLocator::instance().get<ScanService>()->scanMusicNow();
}

void AppContext::cancelScan() {
    ServiceLocator::instance().get<ScanService>()->cancelScan();
}

void AppContext::importCollectionFolder(const QString& folderPath, const QString& name) {
    ServiceLocator::instance().get<ScanService>()->importCollectionFolder(folderPath, name);
}

void AppContext::addToLocalMusic(const QString& musicHash) {
    ServiceLocator::instance().get<ScanService>()->addToLocalMusic(musicHash);
}

void AppContext::savePlaybackState() {
    ServiceLocator::instance().get<PlaybackService>()->savePlaybackState();
}

void AppContext::restorePlaybackState() {
    ServiceLocator::instance().get<PlaybackService>()->restorePlaybackState();
}

QVariantList AppContext::findDuplicateSongs() {
    return ServiceLocator::instance().get<DedupService>()->findDuplicateSongs();
}

void AppContext::resolveDuplicate(const QString& keepHash, const QStringList& deleteHashes) {
    ServiceLocator::instance().get<DedupService>()->resolveDuplicate(keepHash, deleteHashes);
    emit deletedMusicCountChanged();
}

QVariantMap AppContext::getSyncConfig() const {
    return AppConfig::instance().getSyncConfig();
}

void AppContext::setSyncConfig(const QVariantMap& cfg) {
    AppConfig::instance().setSyncConfig(cfg);
}

// ===== connectSyncSignals =====
// Migrate to ServiceLocator pattern - use get<>() to access services lazily
void AppContext::connectSyncSignals()
{
    connect(ServiceLocator::instance().get<SyncManager>(), &SyncManager::syncProgress, this, [this](int cur, int total, const QString& file) {
        emit syncProgressChanged(cur, total, file);
    });
}

void AppContext::startSync()
{
    if (m_syncRunning) {
        Log.debug("[Sync] already running, skip duplicate start");
        return;
    }

    if (m_syncThread && m_syncThread->isRunning()) {
        Log.debug("[Sync] backend thread still running, skip");
        return;
    }

    if (ServiceLocator::instance().get<ScanService>()->isScanning()) {
        Log.info("[Sync] scanning in progress, skip sync");
        return;
    }

    if (!AppConfig::instance().syncConfig().enabled) {
        Log.debug("[Sync] sync not enabled");
        emit syncError(QString::fromUtf8("The sync is not enabled, please enable it in the settings."));
        return;
    }

    m_syncRunning = true;
    emit syncRunningChanged();
    QCoreApplication::processEvents();

    Log.debug("[Sync] ================== Sync Start ==================");


    FtpConfig ftpCfg;
    ftpCfg.host = AppConfig::instance().syncConfig().nasHost;
    ftpCfg.port = AppConfig::instance().syncConfig().nasPort;
    ftpCfg.user = AppConfig::instance().syncConfig().nasUser;
    ftpCfg.password = AppConfig::instance().syncConfig().nasPassword;
    ftpCfg.syncRoot = AppConfig::instance().syncConfig().nasSyncRoot;

    Log.debug(QString("[Sync] FTP config: %1:%2 user=%3 root=%4")
        .arg(ftpCfg.host).arg(ftpCfg.port).arg(ftpCfg.user, ftpCfg.syncRoot));


    Log.debug("[Sync] Building local manifest...");
    emit syncPhaseChanged(1, "Building local manifest");

    ServiceLocator::instance().get<SyncManager>()->setSyncConfig(AppConfig::instance().syncConfig());
    SyncManifest localManifest = ServiceLocator::instance().get<SyncManager>()->buildLocalManifest();
    Log.debug(QString("[Sync] Local manifest: files=%1 covers=%2")
        .arg(localManifest.files.size()).arg(localManifest.covers.size()));

    // Build file_hash to local absolute path mapping
    QMap<QString, QString> hashToPath;
    for (const auto& item : ServiceLocator::instance().get<MusicFileManager>()->getMusicList()) {
        if (!item.fileHash.isEmpty()) {
            if (QFile::exists(item.filePath)) {
                hashToPath[item.fileHash] = item.filePath;
            } else {
                Log.warning(QString("[Sync] hashToPath — 文件已不存在，跳过: %1 [%2]")
                    .arg(item.filePath, item.fileHash.left(8)));
            }
        }
    }

    // Collection playlist songs paths (is_local=0 in playlist_items, not in getMusicList)
    if (ServiceLocator::instance().get<PlaylistManager>()) {
        const auto& collections = ServiceLocator::instance().get<PlaylistManager>()->getCollectionPlaylists();
        for (const auto& pl : collections) {
            const auto& items = ServiceLocator::instance().get<PlaylistManager>()->getPlaylistItems(pl.id);
            for (const auto& pi : items) {
                if (!pi.musicHash.isEmpty() && !hashToPath.contains(pi.musicHash)) {
                    MusicItem mi = ServiceLocator::instance().get<MusicFileManager>()->loadMusicItemByHash(pi.musicHash);
                    if (!mi.fileHash.isEmpty()) {
                        if (QFile::exists(mi.filePath)) {
                            hashToPath[mi.fileHash] = mi.filePath;
                        } else {
                            Log.warning(QString("[Sync] hashToPath(Collection) — 文件已不存在，跳过: %1 [%2]")
                                .arg(mi.filePath, mi.fileHash.left(8)));
                        }
                    }
                }
            }
        }
    }

    // Collect all deleted songs (both devices use same account, no confirmation needed)
    QList<DeletedFileLogEntry> confirmedDeletions;
    auto deletedList = ServiceLocator::instance().get<MusicFileManager>()->getDeletedMusicList();
    confirmedDeletions.reserve(deletedList.size());
    for (const auto& var : deletedList) {
        QVariantMap v = var.toMap();
        DeletedFileLogEntry log;
        log.fileHash = v["fileHash"].toString();
        log.deletedAt = v["deletedAt"].toLongLong();
        confirmedDeletions.append(log);
    }
    Log.info(QString("[Sync] Deleted songs: %1").arg(confirmedDeletions.size()));

    // Create background thread worker
    auto* worker = new SyncWorker();
    m_activeWorker = worker;
    worker->setConcurrency(4);
    worker->setFtpConfig(ftpCfg);
    worker->setSyncConfig(AppConfig::instance().syncConfig());
    worker->setLocalManifest(localManifest);
    worker->setConfirmedDeletions(confirmedDeletions);
    worker->setLocalPathMap(hashToPath);
    worker->setKnownLocalHashes(ServiceLocator::instance().get<MusicFileManager>()->getAllMusicIds());

    // Pre-export sync data via SyncCoordinator (main thread, DB access safe)
    QByteArray syncDataJson = ServiceLocator::instance().get<SyncCoordinator>()->exportAll();
    worker->setLocalSyncDataJson(syncDataJson);
    worker->setCoordinator(ServiceLocator::instance().get<SyncCoordinator>());

    Log.debug(QString("[Sync] Pre-export sync_data.json: %1 bytes").arg(syncDataJson.size()));

    m_syncThread = new QThread(this);
    worker->moveToThread(m_syncThread);

    // Connect signals (cross-thread queued connection)
    connect(worker, &SyncWorker::phaseChanged, this, [this](int phase, const QString& name) {
        emit syncPhaseChanged(phase, name);
    });
    connect(worker, &SyncWorker::progressChanged, this, [this](int cur, int total, const QString& file) {
        emit syncProgressChanged(cur, total, file);
    });
    connect(worker, &SyncWorker::errorOccurred, this, [this](const QString& msg) {
        Log.debug(QString("[Sync] Error: %1").arg(msg));
        emit syncError(msg);
        m_syncRunning = false;
        emit syncRunningChanged();
    });
    connect(worker, &SyncWorker::finished, this, [this, worker](int totalOps, int conflicts, int failedCount, QStringList failedFiles) {
        Log.debug(QString("[Sync] Done: totalOps=%1 conflicts=%2 failed=%3")
            .arg(totalOps).arg(conflicts).arg(failedCount));
        for (const auto& f : failedFiles) {
            Log.debug(QString("[Sync]   Failed: %1").arg(f));
        }

        QByteArray remoteSyncData = worker->mergedSyncData();

        auto downloadedFiles = worker->downloadedFiles();
        if (!downloadedFiles.isEmpty()) {
            importDownloadedFiles(downloadedFiles, worker->mergedManifest());
        }

        ensurePlaylistFilesExist(remoteSyncData, worker->mergedManifest());
        ServiceLocator::instance().get<MusicFileManager>()->refreshMusicList();

        ServiceLocator::instance().get<SyncCoordinator>()->applyAll(remoteSyncData);

        ServiceLocator::instance().get<MusicFileManager>()->refreshMusicList();

        if (auto* vmm = ServiceLocator::instance().get<ViewModelManager>()) {
            if (auto* lvm = vmm->getViewModel<LocalMusicViewModel>("local_music")) {
                auto& list = ServiceLocator::instance().get<MusicFileManager>()->getMusicList();
                QList<MusicItem> items(list.begin(), list.end());
                applyMetadataOverrides(items);
                updateCoverPaths(items);
                lvm->refreshFromDB(items);
            }
        }

        AppConfig::instance().setLastSyncTime(QDateTime::currentSecsSinceEpoch());

        QString downloadDir = PlatformPathHelper::downloadDir();
        if (QDir(downloadDir).exists()
            && !QDir(downloadDir).entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files).isEmpty()) {
            addMusicScanDirectory(downloadDir);
        }

        emit syncCompleted(totalOps, conflicts);
        m_syncRunning = false;
        emit syncRunningChanged();

        worker->deleteLater();
        m_syncThread->quit();
    });

    connect(m_syncThread, &QThread::started, worker, &SyncWorker::doSync);
    connect(m_syncThread, &QThread::finished, this, [this]() {
        m_syncThread = nullptr;
        m_activeWorker = nullptr;
    });
    connect(m_syncThread, &QThread::finished, m_syncThread, &QObject::deleteLater);
    connect(m_syncThread, &QThread::finished, worker, &QObject::deleteLater);

    m_syncThread->start();
}

void AppContext::cancelSync()
{
    if (m_activeWorker) {
        m_activeWorker->cancel();
    }
}

void AppContext::exportLogs()
{
#ifdef Q_OS_ANDROID
    const QString srcPath = QString::fromStdString(Logger::get_instance().currentLogPath());
    const QFileInfo srcInfo(srcPath);

    if (!srcInfo.exists()) {
        Log.warning(QString("[Log Export] Source file not found: %1").arg(srcPath));
        emit logExported({}, false);
        return;
    }

    // Use external accessible path (PC/USB visible), fallback to internal storage
    const QString destDir = PlatformPathHelper::exportDir();
    const QString destPath = destDir + "/" + srcInfo.fileName();

    if (QFile::exists(destPath)) {
        QFile::remove(destPath);
    }

    if (QFile::copy(srcPath, destPath)) {
        Log.info(QString("[Log Export] Success: %1").arg(destPath));
        emit logExported(destPath, true);
    } else {
        Log.warning(QString("[Log Export] Copy failed: %1 -> %2").arg(srcPath, destPath));
        emit logExported({}, false);
    }
#else
    emit logExported({}, false);
#endif
}

// ===== deleteMusic =====

bool AppContext::deleteMusic(const QString& musicHash, bool deleteLocalFile)
{
    if (musicHash.isEmpty()) {
        Log.error("deleteMusic: empty musicHash");
        return false;
    }

    MusicItem item = ServiceLocator::instance().get<MusicFileManager>()->getMusicItemByFileHash(musicHash);
    const QString filePath  = item.filePath;
    const QString coverHash = item.coverHash;

    bool dbResult = ServiceLocator::instance().get<MusicFileManager>()->deleteMusicByHash(musicHash);
    if (!dbResult) {
        Log.error("deleteMusic: database delete failed for hash:" + musicHash);
        return false;
    }

    if (deleteLocalFile && !filePath.isEmpty()) {
        if (QFile::exists(filePath)) {
            if (!QFile::remove(filePath)) {
                Log.warning("deleteMusic: failed to remove local file:" + filePath);
            } else {
                Log.info("deleteMusic: removed local file:" + filePath);
            }
        }
    }

    auto* coverMgr = ServiceLocator::instance().get<MusicCoverManager>();
    if (!coverHash.isEmpty() && coverMgr) {
        int remainingRefs = ServiceLocator::instance().get<MusicFileManager>()->countByCoverHash(coverHash);
        if (remainingRefs == 0) {
            Log.info(QString("deleteMusic: cover '%1' is no longer referenced, deleting cover file").arg(coverHash));
            coverMgr->deleteCover(coverHash);
        } else {
            Log.info(QString("deleteMusic: cover '%1' still referenced by %2 track(s), keeping").arg(coverHash).arg(remainingRefs));
        }
    }

    emit deletedMusicCountChanged();

    auto* overrideMgr = ServiceLocator::instance().get<MetadataOverrideManager>();
    if (overrideMgr->hasOverrides(musicHash)) {
        QVariantMap overrides = overrideMgr->getOverrides(musicHash);
        if (overrides.contains("cover_hash")) {
            QString ovCoverHash = overrides["cover_hash"].toString();
            if (!ovCoverHash.isEmpty() && coverMgr) {
                int refs = ServiceLocator::instance().get<MusicFileManager>()->countCoverRefsAcrossTables(ovCoverHash, musicHash);
                if (refs == 0) {
                    coverMgr->deleteCover(ovCoverHash);
                }
            }
        }
        overrideMgr->removeOverride(musicHash);
    }

    if (auto* localViewModel = ServiceLocator::instance().get<ViewModelManager>()->getViewModel<LocalMusicViewModel>("local_music")) {
        localViewModel->removeByHash(musicHash);
    }

    emit musicDeleted(musicHash);
    Log.info("deleteMusic: successfully deleted music with hash:" + musicHash);
    return true;
}

// ===== Deleted music management =====

QVariantList AppContext::getDeletedMusicList()
{
    auto* mfm = ServiceLocator::instance().get<MusicFileManager>();
    return mfm ? mfm->getDeletedMusicList() : QVariantList();
}

bool AppContext::restoreDeletedMusic(const QString& fileHash)
{
    if (fileHash.isEmpty()) return false;

    auto* mfm = ServiceLocator::instance().get<MusicFileManager>();
    if (!mfm) return false;

    QVariantMap record = mfm->getMusicFileRecord(fileHash);
    if (record.isEmpty()) {
        Log.warning("restoreDeletedMusic: record not found: " + fileHash);
        return false;
    }

    const QString filePath = record["file_path"].toString();
    if (!QFile::exists(filePath)) {
        Log.warning("restoreDeletedMusic: file missing: " + filePath);
        return false;
    }

    if (!mfm->restoreMusicByHash(fileHash)) {
        Log.warning("restoreDeletedMusic: restore failed: " + fileHash);
        return false;
    }

    MusicItem mi;
    mi.fileHash = record["file_hash"].toString();
    mi.filePath = filePath;
    mi.title = record["title"].toString();
    mi.artist = record["artist"].toString();
    mi.album = record["album"].toString();
    mi.duration = formatDuration(record["length_in_seconds"].toInt());
    mi.coverHash = record["cover_hash"].toString();
    mi.quality = record["quality"].toString();
    mi.sectionKey = PinyinUtils::sectionKey(mi.title);

    mfm->getMusicList().append(mi);

    if (auto* localViewModel = ServiceLocator::instance().get<ViewModelManager>()->getViewModel<LocalMusicViewModel>("local_music")) {
        localViewModel->appendItem(mi);
    }

    ServiceLocator::instance().get<ViewModelManager>()->getViewModel<PlaylistViewModel>("playlists")->refresh();
    emit deletedMusicCountChanged();
    Log.info(QString("Restored music: %1").arg(mi.title));
    return true;
}

int AppContext::restoreDeletedMusicBatch(const QStringList& hashes)
{
    int success = 0;
    for (const QString& hash : hashes) {
        if (restoreDeletedMusic(hash)) {
            success++;
        }
    }
    return success;
}

void AppContext::removeDeletedEntryRecord(const QString& fileHash)
{
    auto* mfm = ServiceLocator::instance().get<MusicFileManager>();
    if (mfm) {
        mfm->permanentlyRemoveMusic(fileHash);
    }
    emit deletedMusicCountChanged();
}

void AppContext::clearAllDeletedEntries()
{
    auto* mfm = ServiceLocator::instance().get<MusicFileManager>();
    if (!mfm) return;

    auto deletedList = mfm->getDeletedMusicList();
    for (const auto& var : deletedList) {
        QVariantMap v = var.toMap();
        QString hash = v["fileHash"].toString();
        if (v["fileExists"].toBool()) {
            mfm->restoreMusicByHash(hash);
        } else {
            mfm->permanentlyRemoveMusic(hash);
        }
    }
    emit deletedMusicCountChanged();
}

// ===== Duplicates =====

QVariantMap AppContext::getSongOverrides(const QString& fileHash)
{
    auto* overrideMgr = ServiceLocator::instance().get<MetadataOverrideManager>();
    if (!overrideMgr) return {};
    return overrideMgr->getOverrides(fileHash);
}

bool AppContext::setSongMetadata(const QString& fileHash, const QString& title,
                                  const QString& artist, const QString& album,
                                  const QString& genre, int year)
{
    auto* overrideMgr = ServiceLocator::instance().get<MetadataOverrideManager>();
    if (fileHash.isEmpty() || !overrideMgr) return false;

    QVariantMap data;
    data["title"]  = title;
    data["artist"] = artist;
    data["album"]  = album;
    data["genre"]  = genre;
    data["year"]   = (year > 0) ? QVariant(year) : QVariant();

    overrideMgr->setMetadata(fileHash, data);

    auto& list = ServiceLocator::instance().get<MusicFileManager>()->getMusicList();
    for (auto& item : list) {
        if (item.fileHash == fileHash) {
            if (!title.isEmpty())  item.title = title;
            if (!artist.isEmpty()) item.artist = artist;
            if (!album.isEmpty())  item.album = album;
            break;
        }
    }

    if (auto* vm = ServiceLocator::instance().get<ViewModelManager>()->getViewModel<LocalMusicViewModel>("local_music")) {
        vm->updateMetadataByHash(fileHash, title, artist, album);
    }
    return true;
}

bool AppContext::setSongCover(const QString& fileHash, const QString& imagePath)
{
    auto* overrideMgr = ServiceLocator::instance().get<MetadataOverrideManager>();
    if (fileHash.isEmpty() || imagePath.isEmpty() || !overrideMgr) return false;

    QImage source(imagePath);
    if (source.isNull()) {
        Log.error("Failed to load cover image: " + imagePath);
        return false;
    }

    QImage normalized = MusicCoverManager::prepareCoverImage(source);

    auto* coverMgr = ServiceLocator::instance().get<MusicCoverManager>();
    QString coverHash = coverMgr->saveCoverSafe(normalized);
    if (coverHash.isEmpty()) return false;

    coverMgr->persistCoverToDb(coverHash);

    overrideMgr->setCoverHash(fileHash, coverHash);

    QUrl coverUrl;
    auto& list = ServiceLocator::instance().get<MusicFileManager>()->getMusicList();
    for (auto& item : list) {
        if (item.fileHash == fileHash) {
            item.coverHash = coverHash;
            item.coverPath = QUrl::fromLocalFile(
                coverMgr->getCoverPath(coverHash));
            coverUrl = item.coverPath;
            break;
        }
    }

    if (auto* vm = ServiceLocator::instance().get<ViewModelManager>()->getViewModel<LocalMusicViewModel>("local_music")) {
        vm->updateCoverByHash(fileHash, coverHash, coverUrl);
    }
    return true;
}

QString AppContext::cropCoverToSquare(const QString& sourcePath, int cropX, int cropY, int cropSize)
{
    if (sourcePath.isEmpty() || cropSize <= 0) return {};

    QImage source(sourcePath);
    if (source.isNull()) {
        Log.error("cropCoverToSquare: failed to load image: " + sourcePath);
        return {};
    }

    cropX = qMax(0, qMin(cropX, source.width() - 1));
    cropY = qMax(0, qMin(cropY, source.height() - 1));
    int maxSize = qMin(source.width() - cropX, source.height() - cropY);
    cropSize = qMin(cropSize, maxSize);

    QImage cropped = source.copy(cropX, cropY, cropSize, cropSize);
    QImage normalized = MusicCoverManager::prepareCoverImage(cropped);

    QString tempDir = PlatformPathHelper::cacheDir();
    QDir().mkpath(tempDir);
    QString tempPath = tempDir + "/crop_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".jpg";

    if (!normalized.save(tempPath, "JPG", 85)) {
        Log.error("cropCoverToSquare: failed to save cropped image to: " + tempPath);
        return {};
    }

    return tempPath;
}

bool AppContext::resetSongMetadata(const QString& fileHash)
{
    auto* overrideMgr = ServiceLocator::instance().get<MetadataOverrideManager>();
    if (fileHash.isEmpty() || !overrideMgr) return false;

    overrideMgr->removeOverride(fileHash);

    QVariantMap restoredValues;
    auto& list = ServiceLocator::instance().get<MusicFileManager>()->getMusicList();
    for (auto& item : list) {
        if (item.fileHash == fileHash) {
            QueryOptions opt;
            opt.setColumns({"title", "artist", "album", "cover_hash"});
            opt.setWhere("file_hash = :hash", {{"hash", fileHash}});
            auto rows = ServiceLocator::instance().get<SqliteDatabaseService>()->select("music_files", opt);
            if (!rows.isEmpty()) {
                restoredValues = rows[0];
                item.title  = rows[0]["title"].toString();
                item.artist = rows[0]["artist"].toString();
                item.album  = rows[0]["album"].toString();
                QString origHash = rows[0]["cover_hash"].toString();
                item.coverHash = origHash;
                if (!origHash.isEmpty()) {
                    item.coverPath = QUrl::fromLocalFile(
                        ServiceLocator::instance().get<MusicCoverManager>()->getCoverPath(origHash));
                } else {
                    item.coverPath = QUrl();
                }
            }
            break;
        }
    }

    if (auto* vm = ServiceLocator::instance().get<ViewModelManager>()->getViewModel<LocalMusicViewModel>("local_music")) {
        if (!restoredValues.isEmpty()) {
            vm->updateMetadataByHash(fileHash,
                                     restoredValues["title"].toString(),
                                     restoredValues["artist"].toString(),
                                     restoredValues["album"].toString());
        }
    }
    return true;
}

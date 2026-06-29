#include "init_steps.h"

#include "../application/appconfig.h"
#include "../application/playercontroller.h"
#include "../application/appcontext.h"
#include "../database/sqlite/sqlitedatabaseservice.h"
#include "../filesystem/musicfile/musicfilemanager.h"
#include "../filesystem/musicfile/musicscanner.h"
#include "../filesystem/metadata/musiccovermanager.h"
#include "../filesystem/metadata/metadataoverridemanager.h"
#include "../playlist/playlistmanager.h"
#include "../playlist/favoritemanager.h"
#include "../viewmodels/viewmodelmanager.h"
#include "../viewmodels/music/local_music_view_model.h"
#include "../viewmodels/playlist/playlistviewmodel.h"
#include "../viewmodels/playlist/collectionplaylistviewmodel.h"
#include "../viewmodels/playlist/playlistdetailviewmodel.h"
#include "../services/service_locator.h"
#include "../services/scan_service.h"
#include "../services/dedup_service.h"
#include "../services/playback_service.h"
#include "../utils/logger.h"
#include "../utils/pinyin.h"
#include "../utils/musicinfo.h"
#include "../utils/platform_path_helper.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QTimer>
#include <QCoreApplication>

static QList<MusicItem> s_loadedMusicList;

static void applyMetadataToItem(MusicItem& item, const QVariantMap& ov)
{
    if (ov.contains("title"))    item.title    = ov["title"].toString();
    if (ov.contains("artist"))   item.artist   = ov["artist"].toString();
    if (ov.contains("album"))    item.album    = ov["album"].toString();
    if (ov.contains("cover_hash") && !ov["cover_hash"].isNull())
        item.coverHash = ov["cover_hash"].toString();
}

static QList<MusicItem> s_queryMusicFiles(QSqlDatabase& db)
{
    QList<MusicItem> list;
    QSqlQuery query(db);
    QString sql = "SELECT DISTINCT mf.file_hash, mf.file_path, mf.title, mf.artist, mf.album, "
                  "mf.length_in_seconds, mf.cover_hash, mf.quality "
                  "FROM music_files mf "
                  "LEFT JOIN playlist_items pi ON pi.music_hash = mf.file_hash AND pi.is_removed = 0 "
                  "WHERE mf.is_deleted = 0 "
                  "  AND (pi.playlist_id IS NULL OR pi.is_local = 1)";
    query.prepare(sql);
    if (query.exec()) {
        while (query.next()) {
            MusicItem item;
            item.fileHash  = query.value("file_hash").toString();
            item.filePath  = query.value("file_path").toString();
            item.title     = query.value("title").toString();
            item.artist    = query.value("artist").toString();
            item.album     = query.value("album").toString();
            item.duration  = formatDuration(query.value("length_in_seconds").toInt());
            item.coverHash = query.value("cover_hash").toString();
            item.quality   = query.value("quality").toString();
            list.append(item);
        }
    }
    return list;
}

// ===== PreQml 步骤实现 =====

void DatabaseInitStep::execute()
{
    auto& cfg = AppConfig::instance();
    auto* db = ServiceLocator::instance().registerService<SqliteDatabaseService>(cfg.databasePath());
    // 将解析后的实际路径回写到 AppConfig（config.json 可能为空，SqliteDatabaseService 已解析默认路径）
    QString resolvedPath = db->getDatabase().databaseName();
    if (!resolvedPath.isEmpty() && cfg.databasePath() != resolvedPath) {
        cfg.setDatabasePath(resolvedPath);
    }
    Log.info("DatabaseInit complete");
}

void PlayerInitStep::execute()
{
    auto& pc = PlayerController::instance();
    ServiceLocator::instance().registerExisting(pc);
    Log.info("PlayerInit complete");
}

void SchemaInitStep::execute()
{
    auto& loc = ServiceLocator::instance();
    auto* db  = loc.get<SqliteDatabaseService>();

    loc.registerService<MusicFileManager>(static_cast<QObject*>(nullptr), db);
    loc.registerService<MusicCoverManager>(static_cast<QObject*>(nullptr), db);
    loc.registerService<MetadataOverrideManager>(static_cast<QObject*>(nullptr), db);

    loc.get<MusicFileManager>()->initialize();
    loc.get<MusicCoverManager>()->initialize();
    loc.get<MetadataOverrideManager>()->initialize();
    Log.info("SchemaInit complete");
}

void PlaylistInitStep::execute()
{
    auto& loc = ServiceLocator::instance();
    auto* db  = loc.get<SqliteDatabaseService>();
    auto* pm  = loc.registerService<PlaylistManager>(db);

    auto& fm = FavoriteManager::instance();
    fm.initialize(pm);
    loc.registerExisting(fm);
    Log.info("PlaylistInit complete");
}

void ViewModelRegisterStep::execute()
{
    auto& loc = ServiceLocator::instance();
    auto* pm  = loc.get<PlaylistManager>();
    auto* vmMgr = loc.registerService<ViewModelManager>();

    vmMgr->registerViewModelType<LocalMusicViewModel>("LocalMusic");
    vmMgr->registerPlaylistViewModelType<PlaylistViewModel>("Playlist", pm);
    vmMgr->registerPlaylistViewModelType<CollectionPlaylistViewModel>("CollectionPlaylist", pm);
    vmMgr->registerPlaylistViewModelType<PlaylistDetailViewModel>("PlaylistDetail", pm);

    vmMgr->createViewModel("LocalMusic", "local_music");
    vmMgr->createViewModel("Playlist", "playlists");
    vmMgr->createViewModel("CollectionPlaylist", "collection_playlists");
    vmMgr->createViewModel("PlaylistDetail", "playlist_detail");

    vmMgr->initialize();
    ServiceLocator::instance().markInitialized();
    Log.info("ViewModelRegister complete");
}

// ===== Async 步骤实现（后台线程）=====

void DataLoadInitStep::execute()
{
    QSqlDatabase workerDb = QSqlDatabase::database("init_worker");
    if (!workerDb.isOpen()) {
        Log.error("[DataLoadInitStep] worker DB not open");
        return;
    }

    s_loadedMusicList = s_queryMusicFiles(workerDb);

    if (s_loadedMusicList.isEmpty()) {
        Log.info("[DataLoadInitStep] music_files is empty");
        return;
    }

    for (auto& item : s_loadedMusicList) {
        item.sectionKey = PinyinUtils::sectionKey(
            item.title.isEmpty() ? QFileInfo(item.filePath).completeBaseName() : item.title);
        item.pinyinFull = PinyinUtils::toPinyin(
            item.title.isEmpty() ? QFileInfo(item.filePath).completeBaseName() : item.title).toLower();
    }

    Log.info(QString("[DataLoadInitStep] loaded %1 songs").arg(s_loadedMusicList.size()));
}

// ===== PostQml 步骤实现（主线程，信号触发）=====

void ViewModelPopulateStep::execute()
{
    auto* mfm = ServiceLocator::instance().get<MusicFileManager>();
    auto* vmm = ServiceLocator::instance().get<ViewModelManager>();
    if (!mfm || !vmm) return;

    auto& mfmList = mfm->getMusicList();
    Log.info(QString("[ViewModelPopulate] Before overwrite: m_musicList.size()=%1, s_loadedMusicList.size()=%2")
        .arg(mfmList.size()).arg(s_loadedMusicList.size()));

    mfmList = s_loadedMusicList;

    Log.info(QString("[ViewModelPopulate] After overwrite: m_musicList.size()=%1").arg(mfmList.size()));

    // 从主线程缓存补全封面路径和元数据覆写
    auto* cm = ServiceLocator::instance().get<MusicCoverManager>();
    auto* om = ServiceLocator::instance().get<MetadataOverrideManager>();
    if (cm || om) {
        auto overrides = om ? om->getAllOverrides() : QHash<QString, QVariantMap>();
        int coverMatched = 0;
        for (auto& item : mfmList) {
            if (om && overrides.contains(item.fileHash))
                applyMetadataToItem(item, overrides[item.fileHash]);
            if (cm) {
                const QString coverKey = !item.coverHash.isEmpty() ? item.coverHash : item.coverPath.fileName();
                if (!coverKey.isEmpty()) {
                    QString path = cm->getCoverPath(coverKey);
                    if (!path.isEmpty()) {
                        item.coverPath = QUrl::fromLocalFile(path);
                        coverMatched++;
                    }
                }
            }
        }
        Log.info(QString("[ViewModelPopulate] metadata overrides=%1, cover matched: %2/%3")
            .arg(overrides.size()).arg(coverMatched).arg(mfmList.size()));
    }

    if (!mfmList.isEmpty()) {
        QTimer::singleShot(0, mfm, &MusicFileManager::startAsyncValidation);
    }

    if (auto* vm = vmm->getViewModel<LocalMusicViewModel>("local_music")) {
        vm->onMetaDataReady(mfmList);
    }

    s_loadedMusicList.clear();
    Log.info("ViewModelPopulate complete");
}

void ServiceInitStep::execute()
{
    auto& loc = ServiceLocator::instance();
    auto* db  = loc.get<SqliteDatabaseService>();
    auto* mfm = loc.get<MusicFileManager>();
    auto* pc  = loc.get<PlayerController>();
    auto* pm  = loc.get<PlaylistManager>();
    auto* cm  = loc.get<MusicCoverManager>();
    auto* om  = loc.get<MetadataOverrideManager>();
    auto* vm  = loc.get<ViewModelManager>();

    loc.registerService<MusicScanner>();

    auto* ss = loc.registerService<ScanService>(db, mfm, loc.get<MusicScanner>(), cm, om, vm);
    ss->setPlaylistManager(pm);
    ss->setPlaylistViewModel(vm->getViewModel<PlaylistViewModel>("playlists"));

    loc.registerService<DedupService>(mfm, cm, vm);
    loc.registerService<PlaybackService>(pc, mfm);

    Log.info("ServiceInit complete");
}

void SignalConnectStep::execute()
{
    auto& loc = ServiceLocator::instance();
    auto& appCtx = AppContext::instance();

    // ScanService → AppContext bridge
    auto* ss = loc.get<ScanService>();
    QObject::connect(ss, &ScanService::scanningChanged,    &appCtx, &AppContext::scanningChanged);
    QObject::connect(ss, &ScanService::scanPhaseChanged,   &appCtx, &AppContext::scanPhaseChanged);
    QObject::connect(ss, &ScanService::scanProgressChanged, &appCtx, &AppContext::scanProgressChanged);
    QObject::connect(ss, &ScanService::scanCompleted,      &appCtx, &AppContext::scanCompleted);
    QObject::connect(ss, &ScanService::collectionImported, &appCtx, &AppContext::collectionImported);

    // Sync 信号连接
    appCtx.connectSyncSignals();

    // MusicFileManager::validationComplete → LocalMusicViewModel
    if (auto* localVM = loc.get<ViewModelManager>()->getViewModel<LocalMusicViewModel>("local_music")) {
        QObject::connect(loc.get<MusicFileManager>(), &MusicFileManager::validationComplete,
                         localVM, &LocalMusicViewModel::onValidationComplete);
    }

#ifndef Q_OS_ANDROID
    PlatformPathHelper::setDownloadPath(AppConfig::instance().downloadPath());
#endif

    // 自动扫描检查
    if (AppConfig::instance().autoScanMusicOnStartup()) {
        bool dbEmpty = loc.get<MusicFileManager>()->getMusicList().isEmpty();
        Log.info(QString("[SignalConnect] autoScan=%1, m_musicList.isEmpty()=%2 (size=%3)")
            .arg(AppConfig::instance().autoScanMusicOnStartup())
            .arg(dbEmpty ? "true" : "false")
            .arg(loc.get<MusicFileManager>()->getMusicList().size()));
        if (dbEmpty) {
            Log.info("Auto scan enabled and DB is empty, scheduling full scan");
            QTimer::singleShot(500, &appCtx, [&appCtx]() {
                appCtx.scanMusicNow();
            });
        }
    }

    Log.info("SignalConnect complete");
}

void RestorePlaybackStep::execute()
{
    QTimer::singleShot(200, []() {
        auto* ps = ServiceLocator::instance().get<PlaybackService>();
        if (ps) ps->restorePlaybackState();
    });
    Log.info("RestorePlayback scheduled");
}

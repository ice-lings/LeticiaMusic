#include "service_locator.h"
#include "../application/appconfig.h"
#include "../database/sqlite/sqlitedatabaseservice.h"
#include "../filesystem/musicfile/musicfilemanager.h"
#include "../filesystem/musicfile/musicscanner.h"
#include "../filesystem/metadata/musiccovermanager.h"
#include "../filesystem/metadata/metadataoverridemanager.h"
#include "../playlist/playlistmanager.h"
#include "../playlist/favoritemanager.h"
#include "../application/playercontroller.h"
#include "../viewmodels/viewmodelmanager.h"
#include "../viewmodels/music/local_music_view_model.h"
#include "../viewmodels/playlist/playlistviewmodel.h"
#include "../viewmodels/playlist/collectionplaylistviewmodel.h"
#include "../viewmodels/playlist/playlistdetailviewmodel.h"
#include "scan_service.h"
#include "dedup_service.h"
#include "playback_service.h"
#include "../sync/ftp_session.h"
#include "../sync/sync_manager.h"
#include "../sync/sync_coordinator.h"
#include "../sync/deleted_entries_syncable.h"
#include "../utils/logger.h"

void ServiceLocator::initialize()
{
    if (m_initialized) return;

    buildInfrastructure();
    buildManagers();
    buildViewModels();
    buildServices();

    m_initialized = true;
    Log.info("[ServiceLocator] All services initialized");
}

void ServiceLocator::buildInfrastructure()
{
    auto& cfg = AppConfig::instance();
    registerService<SqliteDatabaseService>(cfg.databasePath());

    auto& pc = PlayerController::instance();
    m_services[pc.staticMetaObject.className()] = &pc;
    pc.setParent(this);

    Log.info("[ServiceLocator] Infrastructure layer ready");
}

void ServiceLocator::buildManagers()
{
    auto* db = get<SqliteDatabaseService>();

    registerService<MusicFileManager>(static_cast<QObject*>(nullptr), db);
    registerService<MusicScanner>();
    registerService<MusicCoverManager>(static_cast<QObject*>(nullptr), db);
    registerService<MetadataOverrideManager>(static_cast<QObject*>(nullptr), db);
    registerService<PlaylistManager>(db);

    get<MusicFileManager>()->initialize();
    get<MusicCoverManager>()->initialize();
    get<MetadataOverrideManager>()->initialize();

    auto& fm = FavoriteManager::instance();
    m_services[fm.staticMetaObject.className()] = &fm;
    fm.setParent(this);

    Log.info("[ServiceLocator] Manager layer ready");
}

void ServiceLocator::buildViewModels()
{
    auto* pm = get<PlaylistManager>();
    auto* vmMgr = registerService<ViewModelManager>();

    vmMgr->registerViewModelType<LocalMusicViewModel>("LocalMusic");
    vmMgr->registerPlaylistViewModelType<PlaylistViewModel>("Playlist", pm);
    vmMgr->registerPlaylistViewModelType<CollectionPlaylistViewModel>("CollectionPlaylist", pm);
    vmMgr->registerPlaylistViewModelType<PlaylistDetailViewModel>("PlaylistDetail", pm);

    vmMgr->createViewModel("LocalMusic", "local_music");
    vmMgr->createViewModel("Playlist", "playlists");
    vmMgr->createViewModel("CollectionPlaylist", "collection_playlists");
    vmMgr->createViewModel("PlaylistDetail", "playlist_detail");

    vmMgr->initialize();
    Log.info("[ServiceLocator] ViewModel layer ready");
}

void ServiceLocator::buildServices()
{
    auto* db  = get<SqliteDatabaseService>();
    auto* mfm = get<MusicFileManager>();
    auto* sc  = get<MusicScanner>();
    auto* cm  = get<MusicCoverManager>();
    auto* om  = get<MetadataOverrideManager>();
    auto* vm  = get<ViewModelManager>();
    auto* pc  = get<PlayerController>();
    auto* pm  = get<PlaylistManager>();

    auto* ss = registerService<ScanService>(db, mfm, sc, cm, om, vm);
    ss->setPlaylistManager(pm);
    ss->setPlaylistViewModel(vm->getViewModel<PlaylistViewModel>("playlists"));

    registerService<DedupService>(mfm, cm, vm);
    registerService<PlaybackService>(pc, mfm);

    Log.info("[ServiceLocator] Service layer ready");
}

void ServiceLocator::buildSyncComponents()
{
    FtpSession::initGlobal();
    if (m_services.contains("SyncManager")) return;

    auto* sm  = registerService<SyncManager>();
    auto* sc  = registerService<SyncCoordinator>();

    auto* pm  = get<PlaylistManager>();
    auto* om  = get<MetadataOverrideManager>();
    auto* mfm = get<MusicFileManager>();
    auto* cm  = get<MusicCoverManager>();
    auto* ds  = get<DedupService>();

    sm->setMusicFileManager(mfm);
    sm->setMusicCoverManager(cm);
    sm->setPlaylistManager(pm);

    auto* delSync = new DeletedEntriesSyncable(mfm);

    sc->registerSyncable(pm);
    sc->registerSyncable(om);
    sc->registerSyncable(ds);
    sc->registerSyncable(delSync);

    Log.info("[ServiceLocator] Sync components lazily created");
}

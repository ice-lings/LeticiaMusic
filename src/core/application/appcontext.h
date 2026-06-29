#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QPair>
#include "../utils/singleton_holder.h"
#include "appconfig.h"
#include "playercontroller.h"
#include "../playlist/playlistmanager.h"
#include "../playlist/favoritemanager.h"
#include "../viewmodels/playlist/playlistviewmodel.h"
#include "../viewmodels/playlist/playlistdetailviewmodel.h"
#include "../viewmodels/playlist/collectionplaylistviewmodel.h"
#include "../services/service_locator.h"
#include "../viewmodels/viewmodelmanager.h"
#include "../services/scan_service.h"
#include "../services/dedup_service.h"
#include "../services/playback_service.h"
#include "../services/init_orchestrator.h"

class SyncWorker;
class QThread;
struct SyncManifest;

class AppContext : public QObject, public SingletonHolder<AppContext>
{
    Q_OBJECT
    friend class SingletonHolder<AppContext>;
    explicit AppContext(QObject* parent = nullptr);

public:
    Q_PROPERTY(PlayerController* playerController READ playerController CONSTANT)
    Q_PROPERTY(FavoriteManager* favoriteManager READ favoriteManager CONSTANT)
    Q_PROPERTY(PlaylistManager* playlistManager READ playlistManager CONSTANT)
    Q_PROPERTY(PlaylistViewModel* playlistViewModel READ playlistViewModel CONSTANT)
    Q_PROPERTY(CollectionPlaylistViewModel* collectionPlaylistViewModel READ collectionPlaylistViewModel CONSTANT)
    Q_PROPERTY(PlaylistDetailViewModel* playlistDetailViewModel READ playlistDetailViewModel CONSTANT)
    Q_PROPERTY(AppConfig* config READ config CONSTANT)

    static AppContext& instance() { return get_instance(); }
    static void recordStartupTime();
    InitOrchestrator& initOrchestrator() { return *m_initOrch; }

    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(int scanPhase READ scanPhase NOTIFY scanPhaseChanged)
    Q_PROPERTY(int scanCurrent READ scanCurrent NOTIFY scanProgressChanged)
    Q_PROPERTY(int scanTotal READ scanTotal NOTIFY scanProgressChanged)
    Q_PROPERTY(QString scanCurrentFile READ scanCurrentFile NOTIFY scanProgressChanged)
    Q_PROPERTY(QString scanPhaseText READ scanPhaseText NOTIFY scanPhaseChanged)
    Q_PROPERTY(bool syncRunning READ isSyncRunning NOTIFY syncRunningChanged)
    Q_PROPERTY(int deletedMusicCount READ deletedMusicCount NOTIFY deletedMusicCountChanged)

    PlayerController* playerController() const noexcept;
    FavoriteManager* favoriteManager() const noexcept;
    PlaylistManager* playlistManager() const noexcept;
    PlaylistViewModel* playlistViewModel() const noexcept;
    CollectionPlaylistViewModel* collectionPlaylistViewModel() const noexcept;
    PlaylistDetailViewModel* playlistDetailViewModel() const noexcept;
    AppConfig* config() const noexcept;

    Q_INVOKABLE QObject* getViewModel(const QString& name);
    Q_INVOKABLE QVariantMap getMusicItemForFile(const QString& filePath);
    Q_INVOKABLE QString getCoverPathByFileHash(const QString& fileHash);
    Q_INVOKABLE QStringList getMusicScanDirectories() const;
    Q_INVOKABLE void setMusicScanDirectories(const QStringList& paths);
    Q_INVOKABLE void addMusicScanDirectory(const QString& path);
    Q_INVOKABLE void removeMusicScanDirectory(const QString& path);
    Q_INVOKABLE QString getDatabasePath() const;
    Q_INVOKABLE void setDatabasePath(const QString& path);
    Q_INVOKABLE bool getAutoScanMusicOnStartup() const;
    Q_INVOKABLE void setAutoScanMusicOnStartup(bool enabled);
    Q_INVOKABLE bool getResumePlaybackOnStartup() const;
    Q_INVOKABLE void setResumePlaybackOnStartup(bool enabled);
    Q_INVOKABLE int getLastVolume() const;
    Q_INVOKABLE void setLastVolume(int volume);
    Q_INVOKABLE QString getAppVersion() const;
    Q_INVOKABLE void scanMusicNow();
    Q_INVOKABLE void cancelScan();
    Q_INVOKABLE void importCollectionFolder(const QString& folderPath, const QString& name);
    Q_INVOKABLE void addToLocalMusic(const QString& musicHash);
    Q_INVOKABLE int  getMinFileSizeKB() const;
    Q_INVOKABLE void setMinFileSizeKB(int kb);
    Q_INVOKABLE int  getMinDurationSec() const;
    Q_INVOKABLE void setMinDurationSec(int sec);
    Q_INVOKABLE QVariantList getScanFolders() const;
    Q_INVOKABLE void setScanFolderEnabled(const QString& path, bool enabled);
    Q_INVOKABLE void clearScanFolders();
    Q_INVOKABLE void rebuildScanFoldersFromLibrary();
    Q_INVOKABLE void scanSingleDirectory(const QString& path);
    Q_INVOKABLE QString getDownloadPath() const;
#ifndef Q_OS_ANDROID
    Q_INVOKABLE void setDownloadPath(const QString& path);
#endif
    Q_INVOKABLE QString getDefaultDownloadPath() const;
    Q_INVOKABLE void startSync();
    Q_INVOKABLE void cancelSync();
    Q_INVOKABLE QVariantMap getSyncConfig() const;
    Q_INVOKABLE void setSyncConfig(const QVariantMap& cfg);
    Q_INVOKABLE bool deleteMusic(const QString& musicHash, bool deleteLocalFile);
    Q_INVOKABLE QVariantList getDeletedMusicList();
    Q_INVOKABLE bool restoreDeletedMusic(const QString& fileHash);
    Q_INVOKABLE int restoreDeletedMusicBatch(const QStringList& hashes);
    Q_INVOKABLE void removeDeletedEntryRecord(const QString& fileHash);
    Q_INVOKABLE void clearAllDeletedEntries();
    Q_INVOKABLE void savePlaybackState();
    Q_INVOKABLE void restorePlaybackState();
    Q_INVOKABLE void exportLogs();
    Q_INVOKABLE QVariantList findDuplicateSongs();
    Q_INVOKABLE void resolveDuplicate(const QString& keepHash, const QStringList& deleteHashes);
    Q_INVOKABLE QVariantMap getSongOverrides(const QString& fileHash);
    Q_INVOKABLE bool setSongMetadata(const QString& fileHash, const QString& title,
                                      const QString& artist, const QString& album,
                                      const QString& genre, int year);
    Q_INVOKABLE bool setSongCover(const QString& fileHash, const QString& imagePath);
    Q_INVOKABLE QString cropCoverToSquare(const QString& sourcePath, int cropX, int cropY, int cropSize);
    Q_INVOKABLE bool resetSongMetadata(const QString& fileHash);

    Q_INVOKABLE void markFirstFrameRendered();

#ifdef Q_OS_ANDROID
    Q_INVOKABLE void moveToBackground();
#endif

    bool isScanning() const noexcept;
    int scanPhase() const noexcept;
    int scanCurrent() const noexcept;
    int scanTotal() const noexcept;
    QString scanCurrentFile() const noexcept;
    QString scanPhaseText() const noexcept;
    bool isSyncRunning() const noexcept;
    int deletedMusicCount() const noexcept;

    void connectSyncSignals();

signals:
    void scanningChanged();
    void scanPhaseChanged();
    void scanProgressChanged();
    void scanCompleted(int totalAdded);
    void musicDeleted(const QString& musicHash);
    void collectionImported();
    void deletedMusicCountChanged();
    void syncRunningChanged();
    void syncPhaseChanged(int phase, const QString& phaseName);
    void syncProgressChanged(int current, int total, const QString& currentFile);
    void syncCompleted(int totalOps, int conflictsResolved);
    void syncError(const QString& message);
    void syncLargeDeletePending(int count, const QStringList& dirNames);
    void logExported(const QString& path, bool success);

private:
    void registerInitSteps();
    void applyMetadataOverrides(QList<MusicItem>& items);
    void updateCoverPaths(QList<MusicItem>& musicList);
    void importDownloadedFiles(const QList<QPair<QString, QString>>& files,
                               const SyncManifest& manifest);
    void ensurePlaylistFilesExist(const QByteArray& mergedSyncData,
                                  const SyncManifest& manifest);

    bool m_syncRunning = false;
    QThread* m_syncThread = nullptr;
    SyncWorker* m_activeWorker = nullptr;
    InitOrchestrator* m_initOrch = nullptr;
};

#endif

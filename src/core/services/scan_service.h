#ifndef SCAN_SERVICE_H
#define SCAN_SERVICE_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QList>

#include "../utils/music_entity_type.h"

class IDatabaseService;
class MusicFileManager;
class MusicScanner;
class MusicCoverManager;
class MetadataOverrideManager;
class ViewModelManager;
class ScanWorker;
class QThread;
class QTimer;
struct ScanResultItem;

class ScanService : public QObject
{
    Q_OBJECT
public:
    ScanService(IDatabaseService* db, MusicFileManager* mfm,
                MusicScanner* scanner, MusicCoverManager* coverMgr,
                MetadataOverrideManager* overrideMgr,
                ViewModelManager* vmMgr, QObject* parent = nullptr);

    Q_INVOKABLE void scanMusicNow();
    Q_INVOKABLE void cancelScan();
    Q_INVOKABLE void scanSingleDirectory(const QString& path);
    Q_INVOKABLE void importCollectionFolder(const QString& folderPath, const QString& name);
    Q_INVOKABLE void addToLocalMusic(const QString& musicHash);
    void doScanMusicNow();

    QVariantList getScanFolders() const;
    void setScanFolderEnabled(const QString& path, bool enabled);
    void clearScanFolders();
    void rebuildScanFoldersFromLibrary();

    void setPlaylistManager(class PlaylistManager* pm) { m_playlistManager = pm; }
    void setPlaylistViewModel(class PlaylistViewModel* pvm) { m_playlistViewModel = pvm; }

    bool    isScanning() const       { return m_scanning; }
    int     scanPhase() const        { return m_scanPhase; }
    int     scanCurrent() const      { return m_scan_current; }
    int     scanTotal() const        { return m_scan_total; }
    QString scanCurrentFile() const  { return m_scan_currentFile; }
    QString scanPhaseText() const    { return m_scan_phaseText; }

signals:
    void scanningChanged();
    void scanPhaseChanged();
    void scanProgressChanged();
    void scanCompleted(int totalAdded);
    void collectionImported();

private:
    void finishScanFromWorker(int totalAdded, const QList<ScanResultItem>& items);
    void finishCollectionImport(int totalAdded, const QList<ScanResultItem>& items);
    void applyMetadataOverrides(QList<MusicItem>& items);
    void updateCoverPaths(QList<MusicItem>& musicList);

    IDatabaseService*       m_db;
    MusicFileManager*       m_fileManager;
    MusicScanner*           m_scanner;
    MusicCoverManager*      m_coverManager;
    MetadataOverrideManager* m_overrideManager;
    ViewModelManager*       m_vmManager;

    bool    m_scanning = false;
    int     m_scanPhase = 0;
    int     m_scan_current = 0;
    int     m_scan_total = 0;
    QString m_scan_currentFile;
    QString m_scan_phaseText;

    ScanWorker* m_scanWorker = nullptr;
    QThread*    m_scanThread = nullptr;
    QTimer*     m_progressTimer = nullptr;

    bool    m_collectionImportActive = false;
    QString m_pendingCollectionName;
    QString m_pendingCollectionPath;

    class PlaylistManager* m_playlistManager = nullptr;
    class PlaylistViewModel* m_playlistViewModel = nullptr;
};
#endif

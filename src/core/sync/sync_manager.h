#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include <QObject>
#include "sync_manifest.h"
#include "sync_config.h"

class MusicFileManager;
class MusicCoverManager;
class PlaylistManager;

class SyncManager : public QObject
{
    Q_OBJECT

public:
    explicit SyncManager(QObject* parent = nullptr);

    void setMusicFileManager(MusicFileManager* mgr);
    void setMusicCoverManager(MusicCoverManager* mgr);
    void setPlaylistManager(PlaylistManager* mgr);
    void setSyncConfig(const SyncConfig& cfg) { m_syncConfig = cfg; }
    
    SyncManifest buildLocalManifest();

signals:
    void syncProgress(int current, int total, const QString& currentFile);

private:
    MusicFileManager* m_musicFiles = nullptr;
    MusicCoverManager* m_covers = nullptr;
    PlaylistManager* m_playlists = nullptr;

    SyncConfig m_syncConfig;
};

#endif // SYNC_MANAGER_H

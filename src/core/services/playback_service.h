#ifndef PLAYBACK_SERVICE_H
#define PLAYBACK_SERVICE_H

#include <QObject>
#include <QVariantList>
#include "playback_state.h"

class PlayerController;
class MusicFileManager;
class QTimer;

class PlaybackService : public QObject
{
    Q_OBJECT
public:
    PlaybackService(PlayerController* player, MusicFileManager* mfm,
                    QObject* parent = nullptr);

    Q_INVOKABLE void savePlaybackState();
    Q_INVOKABLE void restorePlaybackState();
    void setupAutoSave(PlayerController* player);

    const PlaybackState& playbackState() const { return m_playbackState; }

private:
    QVariantList rebuildQueue(PlaySourceType sourceType, int playlistId);
    QVariantList rebuildQueueFromPlaylist(int playlistId);
    QVariantList rebuildQueueFromFavorites();
    QVariantList rebuildQueueFromLocalMusic();
    int findTargetIndex(const QVariantList& queue, const PlaybackState& state) const;

    PlayerController* m_player;
    MusicFileManager* m_fileManager;
    QTimer*           m_saveTimer = nullptr;
    PlaybackState     m_playbackState;
    bool              m_restoring = false;
};
#endif

#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVector>
#include <QVariantList>
#include "../utils/singleton_holder.h"
#include "../models/playlist.h"
#include "play_source_type.h"    


class PlayerController : public QObject, public SingletonHolder<PlayerController>
{
    Q_OBJECT
    friend class SingletonHolder<PlayerController>;

    explicit PlayerController(QObject *parent = nullptr);

public:
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playbackStateChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int queueCount READ queueCount NOTIFY queueChanged)
    Q_PROPERTY(int mediaStatus READ mediaStatus NOTIFY mediaStatusChanged)
    Q_PROPERTY(PlayMode playMode READ playMode WRITE setPlayMode NOTIFY playModeChanged)
    
    enum PlayMode {
        ListLoop = 0,
        SingleLoop = 1,
        Random = 2
    };
    Q_ENUM(PlayMode)

    bool isPlaying() const;
    QString currentFile() const;
    qint64 position() const;
    qint64 duration() const;
    void setPosition(qint64 position);
    
    int currentIndex() const { return m_currentIndex; }
    int queueCount() const { return m_playQueue.size(); }
    int mediaStatus() const;
    PlayMode playMode() const { return m_playMode; }

    Q_INVOKABLE void playOrPause();
    Q_INVOKABLE void playFile(const QString &filePath);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(qint64 position);
    Q_INVOKABLE void playAt(int index);
    Q_INVOKABLE void playNext();
    Q_INVOKABLE void playPrev();
    Q_INVOKABLE void setPlayQueue(int playlistId, const QVariantList &items);
    Q_INVOKABLE void clearPlayQueue();
    Q_INVOKABLE QVariantList getPlayQueue() const;
    Q_INVOKABLE void setPlayMode(PlayMode mode);
    
    int currentPlaylistId() const { return m_currentPlaylistId; }
    PlaySourceType queueSourceType() const { return m_playQueueSourceType; }
    Q_INVOKABLE void setQueueSourceType(PlaySourceType type) { m_playQueueSourceType = type; }

    void setSuppressAutoPlay(bool suppress) { m_suppressAutoPlay = suppress; }
    bool isSuppressAutoPlay() const { return m_suppressAutoPlay; }

    static PlayerController& instance() { return get_instance(); }

signals:
    void playbackStateChanged(bool isPlaying);
    void currentFileChanged(const QString &filePath);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void errorOccurred(const QString &message);
    void currentIndexChanged(int index);
    void queueChanged();
    void playModeChanged(PlayMode mode);
    void queueCleared();
    void playlistIdChanged();
    void mediaStatusChanged(int status);

private slots:
    void handlePlayerError(QMediaPlayer::Error error, const QString &errorString);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    void setPlaying(bool playing);
    void doAutoPlayNext();

    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    QString m_currentFile;
    bool m_playing = false;
    QVector<PlaylistMusicItem> m_playQueue;
    int m_currentIndex = -1;
    PlayMode m_playMode = ListLoop;
    int m_currentPlaylistId = -1;
    PlaySourceType m_playQueueSourceType = PlaySourceType::LocalMusic;
    int m_lastRandomIndex = -1;
    bool m_suppressAutoPlay = false;
};

#endif // PLAYERCONTROLLER_H
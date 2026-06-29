#include "playercontroller.h"
#include "../utils/logger.h"

#include <QTimer>
#include <QRandomGenerator>
#include <QUrl>
#include <QString>

PlayerController::PlayerController(QObject *parent)
    : QObject{parent}
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);

    m_audioOutput->setVolume(0.8);
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::errorOccurred,
            this, &PlayerController::handlePlayerError);
    
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &PlayerController::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &PlayerController::durationChanged);
    
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, &PlayerController::onPlaybackStateChanged);

    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, &PlayerController::onMediaStatusChanged);
}

bool PlayerController::isPlaying() const
{
    return m_playing;
}

QString PlayerController::currentFile() const
{
    return m_currentFile;
}

qint64 PlayerController::position() const
{
    return m_player->position();
}

qint64 PlayerController::duration() const
{
    return m_player->duration();
}

int PlayerController::mediaStatus() const
{
    return static_cast<int>(m_player->mediaStatus());
}

void PlayerController::setPosition(qint64 position)
{
    m_player->setPosition(position);
}

void PlayerController::seek(qint64 position)
{
    m_player->setPosition(position);
}

void PlayerController::playOrPause()
{
    if (m_currentFile.isEmpty()) {
        emit errorOccurred("No file selected");
        return;
    }

    if (m_playing) {
        m_player->pause();
        setPlaying(false);
    } else {
        m_player->play();
        setPlaying(true);
    }
}

void PlayerController::playFile(const QString &filePath)
{
    if (filePath.isEmpty()) {
        emit errorOccurred("Invalid file path");
        return;
    }

    if (filePath == m_currentFile && m_playing) {
        playOrPause();
        return;
    }

    if (m_playing) {
        m_player->stop();
    }

    m_currentFile = filePath;
    m_player->setSource(QUrl::fromLocalFile(filePath));

    m_player->play();
    setPlaying(true);

    emit currentFileChanged(m_currentFile);
}

void PlayerController::stop()
{
    m_player->stop();
    setPlaying(false);
}

void PlayerController::handlePlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    Q_UNUSED(error)
    Log.error(QString("[Player] 错误: %1").arg(errorString));
    emit errorOccurred(errorString);
    stop();
}

void PlayerController::setPlaying(bool playing)
{
    if (m_playing != playing) {
        m_playing = playing;
        emit playbackStateChanged(m_playing);
    }
}

void PlayerController::playAt(int index)
{
    if (index < 0 || index >= m_playQueue.size()) {
        return;
    }
    
    m_currentIndex = index;
    m_lastRandomIndex = index;
    const PlaylistMusicItem &item = m_playQueue[index];

    Log.debug(QString("[Player] playAt: idx=%1 file=%2 suppress=%3")
        .arg(index).arg(item.filePath).arg(m_suppressAutoPlay));
    
    m_currentFile = item.filePath;
    m_player->setSource(QUrl::fromLocalFile(item.filePath));
    if (!m_suppressAutoPlay) {
        m_player->play();
    }

    emit currentIndexChanged(m_currentIndex);
    emit currentFileChanged(m_currentFile);
}

void PlayerController::playNext()
{
    m_suppressAutoPlay = false;

    Log.debug(QString("[Player] playNext: mode=%1 curIdx=%2 qSize=%3")
        .arg(m_playMode).arg(m_currentIndex).arg(m_playQueue.size()));

    if (m_playQueue.isEmpty()) {
        return;
    }
    
    int nextIndex = -1;
    
    switch (m_playMode) {
    case ListLoop:
        nextIndex = (m_currentIndex + 1) % m_playQueue.size();
        break;
    case SingleLoop:
        nextIndex = m_currentIndex + 1;
        if (nextIndex >= m_playQueue.size()) {
            nextIndex = 0;
        }
        break;
    case Random:
        if (m_playQueue.size() > 1) {
            do {
                nextIndex = QRandomGenerator::global()->bounded(m_playQueue.size());
            } while (nextIndex == m_lastRandomIndex && m_playQueue.size() > 1);
            m_lastRandomIndex = nextIndex;
        } else {
            nextIndex = 0;
        }
        break;
    default:
        Log.warning(QString("[Player] playNext: 未知playMode=%1, 按ListLoop处理").arg(m_playMode));
        nextIndex = (m_currentIndex + 1) % m_playQueue.size();
        break;
    }
    
    if (nextIndex >= 0 && nextIndex < m_playQueue.size()) {
        Log.debug(QString("[Player] playNext: nextIdx=%1 → playAt").arg(nextIndex));
        playAt(nextIndex);
    } else {
        Log.warning(QString("[Player] playNext: nextIdx=%1 越界 (qSize=%2)")
            .arg(nextIndex).arg(m_playQueue.size()));
    }
}

void PlayerController::playPrev()
{
    m_suppressAutoPlay = false;
    if (m_playQueue.isEmpty()) {
        return;
    }
    
    if (m_player->position() > 3000) {
        m_player->setPosition(0);
        return;
    }
    
    int prevIndex = m_currentIndex - 1;
    if (prevIndex < 0) {
        prevIndex = m_playQueue.size() - 1;
    }
    
    playAt(prevIndex);
}

void PlayerController::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState) {
        setPlaying(true);
    } else if (state == QMediaPlayer::PausedState) {
        setPlaying(false);
    }
}

void PlayerController::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    emit mediaStatusChanged(static_cast<int>(status));

    if (status == QMediaPlayer::EndOfMedia) {
        QTimer::singleShot(0, this, &PlayerController::doAutoPlayNext);
    }
}

void PlayerController::doAutoPlayNext()
{
    if (m_playQueue.isEmpty()) {
        return;
    }

    switch (m_playMode) {
    case SingleLoop:
        playAt(m_currentIndex);
        break;
    case ListLoop:
    case Random:
        playNext();
        break;
    default:
        Log.warning(QString("[Player] doAutoPlayNext: 未知playMode=%1, 按ListLoop处理").arg(m_playMode));
        playNext();
        break;
    }
}

void PlayerController::setPlayQueue(int playlistId, const QVariantList &items)
{
    m_suppressAutoPlay = false;
    m_currentPlaylistId = playlistId;
    m_playQueue.clear();
    m_lastRandomIndex = -1;
    
    for (int i = 0; i < items.size(); ++i) {
        QVariantMap map = items[i].toMap();
        PlaylistMusicItem musicItem;
        musicItem.musicHash = map.value("musicHash").toString();
        musicItem.title = map.value("title").toString();
        musicItem.artist = map.value("artist").toString();
        musicItem.album = map.value("album").toString();
        musicItem.duration = map.value("duration").toString();
        musicItem.filePath = map.value("filePath").toString();
        musicItem.coverPath = map.value("coverPath").toString();
        m_playQueue.append(musicItem);
    }
    
    emit playlistIdChanged();
    emit queueChanged();
}

QVariantList PlayerController::getPlayQueue() const
{
    QVariantList result;
    for (const PlaylistMusicItem &item : m_playQueue) {
        QVariantMap map;
        map.insert("musicHash", item.musicHash);
        map.insert("title", item.title);
        map.insert("artist", item.artist);
        map.insert("album", item.album);
        map.insert("duration", item.duration);
        map.insert("filePath", item.filePath);
        map.insert("coverPath", item.coverPath);
        result.append(map);
    }
    return result;
}

void PlayerController::clearPlayQueue()
{
    m_playQueue.clear();
    m_currentIndex = -1;
    emit queueCleared();
}

void PlayerController::setPlayMode(PlayMode mode)
{
    if (mode < ListLoop || mode > Random) {
        Log.warning(QString("[Player] setPlayMode: 无效模式=%1, 已忽略").arg(static_cast<int>(mode)));
        return;
    }
    if (m_playMode != mode) {
        m_playMode = mode;
        emit playModeChanged(m_playMode);
    }
}
#include "playback_service.h"
#include "service_locator.h"
#include "../application/appconfig.h"
#include "../application/playercontroller.h"
#include "../filesystem/musicfile/musicfilemanager.h"
#include "../playlist/playlistmanager.h"
#include "../playlist/favoritemanager.h"
#include "../utils/logger.h"
#include "../models/playlist.h"
#include <QTimer>
#include <QCoreApplication>
#include <QFile>
#include <QVariantMap>
#include <QVariantList>
#include <memory>

static QVariantMap toQueueItem(const PlaylistMusicItem& item)
{
    QVariantMap map;
    map["musicHash"] = item.musicHash;
    map["title"]     = item.title;
    map["artist"]    = item.artist;
    map["album"]     = item.album;
    map["duration"]  = item.duration;
    map["filePath"]  = item.filePath;
    map["coverPath"] = item.coverPath;
    return map;
}

static QVariantMap toQueueItem(const MusicItem& item)
{
    QVariantMap map;
    map["musicHash"] = item.fileHash;
    map["title"]     = item.title;
    map["artist"]    = item.artist;
    map["album"]     = item.album;
    map["duration"]  = item.duration;
    map["filePath"]  = item.filePath;
    map["coverPath"] = item.coverPath.isValid() ? item.coverPath.toString() : "";
    return map;
}

PlaybackService::PlaybackService(PlayerController* player, MusicFileManager* mfm,
                                 QObject* parent)
    : QObject(parent)
    , m_player(player)
    , m_fileManager(mfm)
    , m_saveTimer(new QTimer(this))
{
    setupAutoSave(player);

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
        m_saveTimer->stop();
        if (m_player && m_player->position() > 0) {
            savePlaybackState();
        }
    });
}

void PlaybackService::savePlaybackState()
{
    if (!m_player || m_restoring) return;

    m_playbackState.lastPlayPosition = m_player->position();
    m_playbackState.lastPlayMode     = static_cast<int>(m_player->playMode());
    m_playbackState.lastPlayIndex    = m_player->currentIndex();
    m_playbackState.lastPlaylistId   = m_player->currentPlaylistId();
    m_playbackState.lastPlaySourceType = m_player->queueSourceType();

    const auto& queue = m_player->getPlayQueue();
    if (m_playbackState.lastPlayIndex >= 0
        && m_playbackState.lastPlayIndex < queue.size()) {
        QVariantMap item = queue[m_playbackState.lastPlayIndex].toMap();
        m_playbackState.lastPlayHash     = item["musicHash"].toString();
        m_playbackState.lastPlayFilePath = item["filePath"].toString();
    } else if (!m_player->currentFile().isEmpty()) {
        m_playbackState.lastPlayFilePath = m_player->currentFile();
        m_playbackState.lastPlayHash.clear();
    }

    AppConfig::instance().saveSection(&m_playbackState);

    Log.debug(QString("[Playback] 播放状态已保存: pos=%1 index=%2")
        .arg(m_playbackState.lastPlayPosition)
        .arg(m_playbackState.lastPlayIndex));
}

void PlaybackService::restorePlaybackState()
{
    if (!m_player) {
        Log.warning("[Playback] PlayerController is uninitialized, skipping playback state restoration");
        return;
    }

    AppConfig::instance().loadSection(&m_playbackState);

    if (m_playbackState.lastPlayFilePath.isEmpty()) {
        Log.info("[Playback] 无可恢复的播放状态");
        return;
    }

    m_restoring = true;
    m_player->setSuppressAutoPlay(true);

    // Phase 1: 不需要数据依赖的恢复 —— 立即执行
    m_player->setPlayMode(static_cast<PlayerController::PlayMode>(m_playbackState.lastPlayMode));

    // Phase 2: 需要队列数据的恢复 —— 延迟至下一事件循环，确保数据已就绪
    QTimer::singleShot(0, this, [this]() {
        QVariantList queueItems = rebuildQueue(m_playbackState.lastPlaySourceType,
                                                m_playbackState.lastPlaylistId);
        if (queueItems.isEmpty()) {
            Log.info("[Playback] 队列重建结果为空，跳过恢复");
            m_player->setSuppressAutoPlay(false);
            m_restoring = false;
            return;
        }

        m_player->setQueueSourceType(m_playbackState.lastPlaySourceType);
        m_player->setPlayQueue(m_playbackState.lastPlaylistId, queueItems);
        m_player->setSuppressAutoPlay(true);

        int targetIndex = findTargetIndex(queueItems, m_playbackState);
        if (targetIndex < 0) {
            Log.warning("[Playback] 无法在重建队列中定位上次播放的歌曲");
            m_player->setSuppressAutoPlay(false);
            m_restoring = false;
            savePlaybackState();
            return;
        }

        m_player->playAt(targetIndex);

        const qint64 savedPos = m_playbackState.lastPlayPosition;
        if (savedPos > 0) {
            auto conn = std::make_shared<QMetaObject::Connection>();
            *conn = connect(m_player, &PlayerController::mediaStatusChanged,
                this, [this, savedPos, conn](int status) {
                    if (status == static_cast<int>(QMediaPlayer::LoadedMedia)) {
                        disconnect(*conn);
                        if (m_player) {
                            m_player->setPosition(savedPos);
                            Log.info(QString("[Playback] position restored: %1ms").arg(savedPos));
                        }
                        m_player->setSuppressAutoPlay(false);
                        m_restoring = false;
                        savePlaybackState();
                    }
                });
            QTimer::singleShot(3000, this, [this, conn]() {
                auto c = *conn;
                if (c) {
                    disconnect(c);
                    m_player->setSuppressAutoPlay(false);
                    m_restoring = false;
                    savePlaybackState();
                }
            });
        } else {
            m_player->setSuppressAutoPlay(false);
            m_restoring = false;
        }

        Log.info(QString("[Playback] 播放状态已恢复: srcType=%1 listId=%2 index=%3 pos=%4")
            .arg(playSourceTypeName(m_playbackState.lastPlaySourceType))
            .arg(m_playbackState.lastPlaylistId)
            .arg(targetIndex)
            .arg(savedPos));
    });
}

void PlaybackService::setupAutoSave(PlayerController* player)
{
    if (!player) return;

    m_saveTimer->setInterval(2000);
    m_saveTimer->setSingleShot(true);
    connect(m_saveTimer, &QTimer::timeout, this, &PlaybackService::savePlaybackState);

    connect(player, &PlayerController::currentFileChanged, this, [this]() {
        savePlaybackState();
    });
    connect(player, &PlayerController::positionChanged, this, [this](qint64) {
        m_saveTimer->start();
    });
    connect(player, &PlayerController::playModeChanged, this, [this](PlayerController::PlayMode) {
        savePlaybackState();
    });
    connect(player, &PlayerController::queueChanged, this, [this]() {
        savePlaybackState();
    });
}

QVariantList PlaybackService::rebuildQueue(PlaySourceType sourceType, int playlistId)
{
    switch (sourceType) {
    case PlaySourceType::Playlist:
        return rebuildQueueFromPlaylist(playlistId);
    case PlaySourceType::Favorites:
        return rebuildQueueFromFavorites();
    case PlaySourceType::LocalMusic:
    default:
        return rebuildQueueFromLocalMusic();
    }
}

QVariantList PlaybackService::rebuildQueueFromPlaylist(int playlistId)
{
    QVariantList list;
    auto* pm = ServiceLocator::instance().get<PlaylistManager>();
    if (!pm || playlistId < 0) return list;
    for (const auto& item : pm->getMusicItems(playlistId))
        list.append(toQueueItem(item));
    Log.info(QString("[Playback] rebuildQueueFromPlaylist id=%1 count=%2").arg(playlistId).arg(list.size()));
    return list;
}

QVariantList PlaybackService::rebuildQueueFromFavorites()
{
    QVariantList list;
    for (const auto& item : FavoriteManager::instance().getFavoriteMusicItems())
        list.append(toQueueItem(item));
    Log.info(QString("[Playback] rebuildQueueFromFavorites count=%1").arg(list.size()));
    return list;
}

QVariantList PlaybackService::rebuildQueueFromLocalMusic()
{
    QVariantList list;
    if (!m_fileManager) return list;
    for (const auto& item : m_fileManager->getMusicList())
        list.append(toQueueItem(item));
    Log.info(QString("[Playback] rebuildQueueFromLocalMusic count=%1").arg(list.size()));
    return list;
}

int PlaybackService::findTargetIndex(const QVariantList& queue, const PlaybackState& state) const
{
    if (state.lastPlayIndex >= 0 && state.lastPlayIndex < queue.size()
        && queue[state.lastPlayIndex].toMap()["musicHash"].toString() == state.lastPlayHash) {
        return state.lastPlayIndex;
    }

    for (int i = 0; i < queue.size(); ++i) {
        if (queue[i].toMap()["musicHash"].toString() == state.lastPlayHash)
            return i;
    }

    for (int i = 0; i < queue.size(); ++i) {
        if (queue[i].toMap()["filePath"].toString() == state.lastPlayFilePath)
            return i;
    }

    return -1;
}

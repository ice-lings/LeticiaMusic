#include "favoritemanager.h"
#include "../services/service_locator.h"
#include "../playlist/playlistmanager.h"
#include "../utils/logger.h"

FavoriteManager::FavoriteManager(QObject* parent)
    : QObject(parent)
{
}

void FavoriteManager::initialize(PlaylistManager* pm)
{
    m_playlistManager = pm;
    rebuildCache();
}

void FavoriteManager::rebuildCache()
{
    m_favoriteCache.clear();
    Playlist favorite = getFavoritePlaylist();
    if (favorite.id > 0) {
        QVector<PlaylistItem> items = m_playlistManager->getPlaylistItems(favorite.id);
        for (const auto &item : items) {
            m_favoriteCache.insert(item.musicHash);
        }
    }
    m_cacheValid = true;
    Log.info(QString("[Fav] rebuildCache: count=%1").arg(m_favoriteCache.size()));
}

void FavoriteManager::invalidateCache()
{
    m_cacheValid = false;
}

Playlist FavoriteManager::getFavoritePlaylist()
{
    return m_playlistManager->getFavoritePlaylist();
}

bool FavoriteManager::addToFavorites(const QString &musicHash)
{
    Playlist favorite = getFavoritePlaylist();
    if (favorite.id <= 0) {
        Log.error("Favorite playlist not found");
        return false;
    }

    if (isFavorite(musicHash)) {
        Log.info("Music already in favorites");
        return true;
    }

    bool success = m_playlistManager->addMusicToPlaylist(favorite.id, musicHash);
    if (success) {
        m_playlistManager->touchPlaylistTimestamp(favorite.id);
        m_favoriteCache.insert(musicHash);
        emit favoriteAdded(musicHash);
    }
    return success;
}

bool FavoriteManager::removeFromFavorites(const QString &musicHash)
{
    Playlist favorite = getFavoritePlaylist();
    if (favorite.id <= 0) {
        Log.error("Favorite playlist not found");
        return false;
    }

    if (!isFavorite(musicHash)) {
        Log.warning("[Fav] removeFromFavorites: not in cache, hash=" + musicHash.left(12));
        return false;
    }

    bool success = m_playlistManager->removeMusicFromPlaylist(favorite.id, musicHash);
    Log.info(QString("[Fav] removeFromFavorites: success=%1 hash=%2").arg(success).arg(musicHash.left(12)));
    if (success) {
        m_playlistManager->touchPlaylistTimestamp(favorite.id);
        m_favoriteCache.remove(musicHash);
        emit favoriteRemoved(musicHash);
        Log.info("[Fav] favoriteRemoved emitted");
    } else {
        Log.error("Failed to remove from favorites: " + musicHash);
    }
    return success;
}

bool FavoriteManager::isFavorite(const QString &musicHash)
{
    if (!m_cacheValid) {
        rebuildCache();
    }
    return m_favoriteCache.contains(musicHash);
}

QVector<PlaylistItem> FavoriteManager::getFavoriteItems()
{
    Playlist favorite = getFavoritePlaylist();
    if (favorite.id <= 0) {
        return QVector<PlaylistItem>();
    }
    return m_playlistManager->getPlaylistItems(favorite.id);
}

QVector<PlaylistMusicItem> FavoriteManager::getFavoriteMusicItems()
{
    Playlist favorite = getFavoritePlaylist();
    if (favorite.id <= 0) {
        return QVector<PlaylistMusicItem>();
    }
    return m_playlistManager->getPlaylistMusicItems(favorite.id);
}

bool FavoriteManager::toggleFavorite(const QString &musicHash)
{
    if (isFavorite(musicHash)) {
        return removeFromFavorites(musicHash);
    } else {
        return addToFavorites(musicHash);
    }
}

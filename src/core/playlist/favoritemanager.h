#ifndef FAVORITEMANAGER_H
#define FAVORITEMANAGER_H

#include <QObject>
#include <QVector>
#include <QSet>
#include "../utils/singleton_holder.h"
#include "../models/playlist.h"

class PlaylistManager;

class FavoriteManager : public QObject, public SingletonHolder<FavoriteManager>
{
    Q_OBJECT
    friend class SingletonHolder<FavoriteManager>;

    explicit FavoriteManager(QObject* parent = nullptr);

public:
    bool addToFavorites(const QString &musicHash);
    bool removeFromFavorites(const QString &musicHash);
    bool isFavorite(const QString &musicHash);
    QVector<PlaylistItem> getFavoriteItems();
    QVector<PlaylistMusicItem> getFavoriteMusicItems();
    Playlist getFavoritePlaylist();

    Q_INVOKABLE bool toggleFavorite(const QString &musicHash);
    Q_INVOKABLE bool checkIsFavorite(const QString &musicHash) { return isFavorite(musicHash); }
    Q_INVOKABLE QVector<PlaylistItem> getFavorites() { return getFavoriteItems(); }

    static FavoriteManager& instance() { return get_instance(); }

    void initialize(PlaylistManager* pm);
    void rebuildCache();
    void invalidateCache();

    QSet<QString> favoriteHashes() const { return m_favoriteCache; }

signals:
    void favoriteAdded(const QString &musicHash);
    void favoriteRemoved(const QString &musicHash);

private:
    PlaylistManager* m_playlistManager = nullptr;
    QSet<QString> m_favoriteCache;
    bool m_cacheValid = false;
};

#endif // FAVORITEMANAGER_H

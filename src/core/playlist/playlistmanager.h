#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QVector>
#include <QVariantList>
#include <QString>
#include <QSet>
#include <QMap>
#include "../models/playlist.h"
#include "../interfaces/idatabaseservice.h"
#include "../sync/isyncable.h"

class PlaylistManager : public QObject, public ISyncable
{
    Q_OBJECT

public:
    static const QString FAVORITE_PLAYLIST_UUID;
    static constexpr int FAVORITE_PLAYLIST_ID = 1;

    explicit PlaylistManager(IDatabaseService* dbService, QObject *parent = nullptr);

    QVector<Playlist> getAllPlaylists();
    Playlist getPlaylist(int id);
    Playlist getPlaylistByUuid(const QString& uuid);
    int createPlaylist(const QString &name, const QString &description = "");
    int createPlaylistWithUuid(const QString& uuid, const QString& name,
                               const QString& description, bool isCollection,
                               const QString& sourceFolder);
    int createCollectionPlaylist(const QString &name, const QString &sourceFolder);
    QVector<Playlist> getCollectionPlaylists();
    Q_INVOKABLE bool clearPlaylistItems(int playlistId);
    bool updatePlaylist(int id, const QString &name, const QString &description);
    void touchPlaylistTimestamp(int playlistId);
    bool renamePlaylist(int id, const QString &newName);
    bool deletePlaylist(int id);
    Q_INVOKABLE bool addMusicToPlaylist(int playlistId, const QString &musicHash, bool isLocal = true);
    Q_INVOKABLE bool addMusicToPlaylistSync(int playlistId, const QString &musicHash);
    Q_INVOKABLE bool addMusicToPlaylistSync(int playlistId, const QString &musicHash, bool isLocal);
    Q_INVOKABLE bool removeMusicFromPlaylist(int playlistId, const QString &musicHash);
    Q_INVOKABLE bool removeMusicFromPlaylistSync(int playlistId, const QString &musicHash);
    Q_INVOKABLE bool restoreMusicToPlaylist(int playlistId, const QString &musicHash);
    QVector<PlaylistItem> getPlaylistItems(int playlistId);
    QVector<PlaylistMusicItem> getPlaylistMusicItems(int playlistId);
    QList<QMap<QString, QVariant>> getPlaylistItemsRaw(int playlistId);
    Q_INVOKABLE Playlist getFavoritePlaylist();
    Q_INVOKABLE int getFavoritePlaylistId();

    Q_INVOKABLE QVector<Playlist> getPlaylists() { return getAllPlaylists(); }
    Q_INVOKABLE Playlist getPlaylistById(int id) { return getPlaylist(id); }
    Q_INVOKABLE int createNewPlaylist(const QString &name, const QString &description = "") { 
        return createPlaylist(name, description); 
    }
    Q_INVOKABLE bool removePlaylist(int id) { return deletePlaylist(id); }
    Q_INVOKABLE QVariantList getItems(int playlistId);
    Q_INVOKABLE QVector<PlaylistMusicItem> getMusicItems(int playlistId) { return getPlaylistMusicItems(playlistId); }
    Q_INVOKABLE bool isMusicFavorite(const QString& musicHash);
    Q_INVOKABLE QVariantList getFavoriteMusicHashes();

    QStringList takePendingDeletedUuids();

    // ISyncable
    QString sectionName() const override { return "playlists"; }
    SyncTask exportSyncData() override;
    SyncTask mergeSyncData(const SyncTask& local, const SyncTask& remote) override;
    bool importSyncData(const SyncTask& task) override;


signals:
    void playlistCreated(int id);
    void playlistUpdated(int id);
    void playlistDeleted(int id);
    void musicAddedToPlaylist(int playlistId, const QString &musicHash);
    void musicRemovedFromPlaylist(int playlistId, const QString &musicHash);
    void favoriteStatusChanged(const QString &musicHash, bool isFavorite);

private:
    bool initializeTables();
    bool createDefaultFavoritePlaylist();

    IDatabaseService* m_dbService = nullptr;
    QSet<QString> m_pendingDeletedUuids;
    const QString PLAYLISTS_TABLE = "playlists";
    const QString PLAYLIST_ITEMS_TABLE = "playlist_items";
};

#endif // PLAYLISTMANAGER_H

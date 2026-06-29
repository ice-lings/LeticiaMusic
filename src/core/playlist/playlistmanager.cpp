#include "playlistmanager.h"
#include "../interfaces/idatabaseservice.h"
#include "../database/sqlite/sqlitedatabaseservice.h"
#include "../utils/logger.h"
#include "favoritemanager.h"
#include "../sync/sync_task.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QDateTime>
#include <QUuid>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>

const QString PlaylistManager::FAVORITE_PLAYLIST_UUID = "00000000-0000-0000-0000-000000000001";

PlaylistManager::PlaylistManager(IDatabaseService* dbService, QObject *parent)
    : QObject(parent), m_dbService(dbService)
{
    initializeTables();
    createDefaultFavoritePlaylist();
}

bool PlaylistManager::initializeTables()
{
    QStringList playlistsColumns = {
        "id INTEGER PRIMARY KEY AUTOINCREMENT",
        "uuid TEXT UNIQUE",
        "name TEXT NOT NULL",
        "description TEXT",
        "cover_path TEXT",
        "is_collection INTEGER DEFAULT 0",
        "source_folder TEXT",
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP",
        "updated_at DATETIME"
    };

    QStringList itemsColumns = {
        "id INTEGER PRIMARY KEY AUTOINCREMENT",
        "playlist_id INTEGER NOT NULL",
        "music_hash TEXT NOT NULL",
        "position INTEGER DEFAULT 0",
        "added_at DATETIME DEFAULT CURRENT_TIMESTAMP",
        "is_removed INTEGER DEFAULT 0",
        "is_local INTEGER DEFAULT 1",
        "updated_at DATETIME",
        "FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE",
        "FOREIGN KEY (music_hash) REFERENCES music_files(file_hash) ON DELETE CASCADE"
    };

    bool playlistsCreated = m_dbService->createTable(PLAYLISTS_TABLE, playlistsColumns);
    bool itemsCreated = m_dbService->createTable(PLAYLIST_ITEMS_TABLE, itemsColumns);

    // Migrate UUID for legacy playlists
    if (playlistsCreated || m_dbService->tableExists(PLAYLISTS_TABLE)) {
        auto records = m_dbService->select(PLAYLISTS_TABLE);
        for (const auto& record : records) {
            if (record.value("uuid").toString().isEmpty()) {
                QVariantMap updateData;
                updateData["uuid"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
                m_dbService->update(PLAYLISTS_TABLE, updateData,
                    "id = :id", {{":id", record.value("id")}});
            }
        }
    }

    return playlistsCreated && itemsCreated;
}

bool PlaylistManager::createDefaultFavoritePlaylist()
{
    QVariantMap values = {
        {"uuid", FAVORITE_PLAYLIST_UUID},
        {"name", "My Favorites"},
        {"description", "My favorite music"}
    };

    m_dbService->insertIgnore(PLAYLISTS_TABLE, values);

    QueryOptions favOptions;
    favOptions.whereClause = "uuid = :uuid";
    favOptions.whereParams = {{":uuid", FAVORITE_PLAYLIST_UUID}};
    auto records = m_dbService->select(PLAYLISTS_TABLE, favOptions);
    if (!records.isEmpty()) {
        int favId = records.first().value("id").toInt();
        Log.info(QString("Favorite playlist ensured, id: %1").arg(favId));
        QString curUpdated = records.first().value("updated_at").toString();
        if (curUpdated.isEmpty()) {
            touchPlaylistTimestamp(favId);
            Log.info(QString("[Sync] createDefaultFavoritePlaylist: initialized NULL updated_at for id=%1").arg(favId));
        }
    }

    return !records.isEmpty();
}

QVector<Playlist> PlaylistManager::getAllPlaylists()
{
    QVector<Playlist> result;
    QueryOptions options;
    options.whereClause = "uuid != :favUuid AND is_collection = 0";
    options.whereParams = {{":favUuid", FAVORITE_PLAYLIST_UUID}};
    options.orderBy = "created_at DESC";
    
    auto records = m_dbService->select(PLAYLISTS_TABLE, options);
    for (const auto &record : records) {
        Playlist playlist;
        playlist.id = record.value("id").toInt();
        playlist.uuid = record.value("uuid").toString();
        playlist.name = record.value("name").toString();
        playlist.description = record.value("description").toString();
        playlist.coverPath = record.value("cover_path").toString();
        playlist.isCollection = record.value("is_collection").toInt() == 1;
        playlist.sourceFolder = record.value("source_folder").toString();
        
        QString createdAtStr = record.value("created_at").toString();
        QString updatedAtStr = record.value("updated_at").toString();

        if (!createdAtStr.isEmpty()) {
            playlist.createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);
            if (!playlist.createdAt.isValid())
                playlist.createdAt = QDateTime::fromString(createdAtStr, "yyyy-MM-dd HH:mm:ss");
        }
        if (!updatedAtStr.isEmpty()) {
            playlist.updatedAt = QDateTime::fromString(updatedAtStr, Qt::ISODate);
            if (!playlist.updatedAt.isValid())
                playlist.updatedAt = QDateTime::fromString(updatedAtStr, "yyyy-MM-dd HH:mm:ss");
        }
        Log.info(QString("[Sync] getAllPlaylists id=%1: updatedAt raw='%2' valid=%3 epoch=%4")
            .arg(playlist.id)
            .arg(updatedAtStr,
                 QString::number(playlist.updatedAt.isValid()),
                 QString::number(playlist.updatedAt.toSecsSinceEpoch())));

        QueryOptions countOptions;
        countOptions.whereClause = "playlist_id = :playlist_id";
        countOptions.whereParams = { {":playlist_id", playlist.id} };
        auto items = m_dbService->select(PLAYLIST_ITEMS_TABLE, countOptions);
        playlist.musicCount = items.size();
        
        result.append(playlist);
    }
    
    return result;
}

Playlist PlaylistManager::getPlaylist(int id)
{
    Playlist playlist;
    QueryOptions options;
    options.whereClause = "id = :id";
    options.whereParams = { {":id", id} };
    
    auto records = m_dbService->select(PLAYLISTS_TABLE, options);
    if (!records.isEmpty()) {
        const auto &record = records.first();
        playlist.id = record.value("id").toInt();
        playlist.uuid = record.value("uuid").toString();
        playlist.name = record.value("name").toString();
        playlist.description = record.value("description").toString();
        playlist.coverPath = record.value("cover_path").toString();
        playlist.isCollection = record.value("is_collection").toInt() == 1;
        playlist.sourceFolder = record.value("source_folder").toString();

        QString createdAtStr = record.value("created_at").toString();
        QString updatedAtStr = record.value("updated_at").toString();
        if (!createdAtStr.isEmpty()) {
            playlist.createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);
            if (!playlist.createdAt.isValid())
                playlist.createdAt = QDateTime::fromString(createdAtStr, "yyyy-MM-dd HH:mm:ss");
        }
        if (!updatedAtStr.isEmpty()) {
            playlist.updatedAt = QDateTime::fromString(updatedAtStr, Qt::ISODate);
            if (!playlist.updatedAt.isValid())
                playlist.updatedAt = QDateTime::fromString(updatedAtStr, "yyyy-MM-dd HH:mm:ss");
        }
        Log.info(QString("[Sync] getPlaylist id=%1: updatedAt raw='%2' valid=%3 epoch=%4")
            .arg(playlist.id)
            .arg(updatedAtStr,
                 QString::number(playlist.updatedAt.isValid()),
                 QString::number(playlist.updatedAt.toSecsSinceEpoch())));

        QueryOptions countOptions;
        countOptions.whereClause = "playlist_id = :playlist_id";
        countOptions.whereParams = { {":playlist_id", playlist.id} };
        auto items = m_dbService->select(PLAYLIST_ITEMS_TABLE, countOptions);
        playlist.musicCount = items.size();
    }
    
    return playlist;
}

Playlist PlaylistManager::getPlaylistByUuid(const QString& uuid)
{
    Playlist playlist;
    QueryOptions options;
    options.whereClause = "uuid = :uuid";
    options.whereParams = { {":uuid", uuid} };
    
    auto records = m_dbService->select(PLAYLISTS_TABLE, options);
    if (!records.isEmpty()) {
        const auto &record = records.first();
        playlist.id = record.value("id").toInt();
        playlist.uuid = record.value("uuid").toString();
        playlist.name = record.value("name").toString();
        playlist.description = record.value("description").toString();
        playlist.coverPath = record.value("cover_path").toString();
        playlist.isCollection = record.value("is_collection").toInt() == 1;
        playlist.sourceFolder = record.value("source_folder").toString();
        
        QueryOptions countOptions;
        countOptions.whereClause = "playlist_id = :playlist_id";
        countOptions.whereParams = { {":playlist_id", playlist.id} };
        auto items = m_dbService->select(PLAYLIST_ITEMS_TABLE, countOptions);
        playlist.musicCount = items.size();
    }
    
    return playlist;
}

int PlaylistManager::createPlaylist(const QString &name, const QString &description)
{
    QVariantMap values = {
        {"uuid", QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {"name", name},
        {"description", description}
    };
    
    int result = m_dbService->insert(PLAYLISTS_TABLE, values);
    if (result >= 0) {
        emit playlistCreated(result);
    }
    return result;
}

int PlaylistManager::createPlaylistWithUuid(const QString& uuid, const QString& name,
                                             const QString& description, bool isCollection,
                                             const QString& sourceFolder)
{
    QVariantMap values = {
        {"uuid", uuid},
        {"name", name},
        {"description", description},
        {"is_collection", isCollection ? 1 : 0},
        {"source_folder", sourceFolder}
    };

    int result = m_dbService->insert(PLAYLISTS_TABLE, values);
    if (result >= 0) {
        Log.info(QString("Playlist created from sync (uuid=%1, name=%2, id=%3)")
            .arg(uuid).arg(name).arg(result));
        emit playlistCreated(result);
    }
    return result;
}

int PlaylistManager::createCollectionPlaylist(const QString &name, const QString &sourceFolder)
{
    QVariantMap values = {
        {"uuid", QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {"name", name},
        {"is_collection", 1},
        {"source_folder", sourceFolder}
    };
    
    m_dbService->insert(PLAYLISTS_TABLE, values);
    
    // Insert with default values, get ID via unique query
    QueryOptions queryOpt;
    queryOpt.whereClause = "name = :name AND is_collection = 1";
    queryOpt.whereParams = {{":name", name}};
    auto records = m_dbService->select(PLAYLISTS_TABLE, queryOpt);
    
    int actualId = -1;
    if (!records.isEmpty()) {
        actualId = records.first().value("id").toInt();
        Log.info(QString("Collection playlist created: %1 (id=%2)").arg(name).arg(actualId));
        emit playlistCreated(actualId);
    } else {
        Log.error(QString("Failed to retrieve collection playlist id for: %1").arg(name));
    }
    
    return actualId;
}

QVector<Playlist> PlaylistManager::getCollectionPlaylists()
{
    QVector<Playlist> result;
    QueryOptions options;
    options.whereClause = "is_collection = 1";
    options.orderBy = "created_at DESC";
    
    auto records = m_dbService->select(PLAYLISTS_TABLE, options);
    for (const auto &record : records) {
        Playlist playlist;
        playlist.id = record.value("id").toInt();
        playlist.uuid = record.value("uuid").toString();
        playlist.name = record.value("name").toString();
        playlist.description = record.value("description").toString();
        playlist.coverPath = record.value("cover_path").toString();
        playlist.isCollection = record.value("is_collection").toInt() == 1;
        playlist.sourceFolder = record.value("source_folder").toString();
        
        QueryOptions countOptions;
        countOptions.whereClause = "playlist_id = :playlist_id";
        countOptions.whereParams = { {":playlist_id", playlist.id} };
        auto items = m_dbService->select(PLAYLIST_ITEMS_TABLE, countOptions);
        playlist.musicCount = items.size();
        
        result.append(playlist);
    }
    
    return result;
}

bool PlaylistManager::updatePlaylist(int id, const QString &name, const QString &description)
{
    QVariantMap data;
    data["name"] = name;
    data["description"] = description;
    data["updated_at"] = QDateTime::currentDateTime();

    const QString whereClause = "id = :id";
    QVariantMap whereParams;
    whereParams["id"] = id;

    const bool ok = m_dbService && m_dbService->update(PLAYLISTS_TABLE, data, whereClause, whereParams);
    if (ok) {
        emit playlistUpdated(id);
    }
    return ok;
}

void PlaylistManager::touchPlaylistTimestamp(int playlistId)
{
    if (!m_dbService || playlistId < 0) return;

    QVariantMap data;
    data["updated_at"] = QDateTime::currentDateTime();
    m_dbService->update(PLAYLISTS_TABLE, data, "id = :id", {{":id", playlistId}});
    Log.info(QString("[Sync] touchPlaylistTimestamp: id=%1 updated_at set to '%2'")
        .arg(playlistId).arg(data["updated_at"].toDateTime().toString("yyyy-MM-dd HH:mm:ss")));
}

bool PlaylistManager::renamePlaylist(int id, const QString &newName)
{
    Log.info(QString("Rename playlist %1 to: %2").arg(id).arg(newName));

    if (id == FAVORITE_PLAYLIST_ID) {
        Log.warning("Cannot rename the favorite playlist");
        return false;
    }

    // Keep existing description if new one is empty
    QString oldDescription;
    {
        QueryOptions options;
        options.whereClause = "id = :id";
        options.whereParams = { {":id", id} };
        auto records = m_dbService->select(PLAYLISTS_TABLE, options);
        if (!records.isEmpty()) {
            oldDescription = records.first().value("description").toString();
        }
    }

    return updatePlaylist(id, newName, oldDescription);
}

bool PlaylistManager::deletePlaylist(int id)
{
    if (id == FAVORITE_PLAYLIST_ID) {
        Log.warning("Cannot delete the favorite playlist");
        return false;
    }
    
    Log.info(QString("Deleting playlist %1").arg(id));

    // Record UUID before deletion
    Playlist pl = getPlaylist(id);
    if (!pl.uuid.isEmpty()) {
        m_pendingDeletedUuids.insert(pl.uuid);
    }
    
    // Logical delete playlist items and remote directories
    QString itemsWhereClause = "playlist_id = :playlist_id";
    QVariantMap itemsWhereParams;
    itemsWhereParams[":playlist_id"] = id;
    
    bool itemsDeleted = m_dbService->remove(PLAYLIST_ITEMS_TABLE, itemsWhereClause, itemsWhereParams);
    if (!itemsDeleted) {
        Log.warning(QString("No items to delete for playlist %1 or deletion failed").arg(id));
        // Do not cascade-delete playlist items on failure
    }
    
    // Delete playlist
    QString playlistWhereClause = "id = :id";
    QVariantMap playlistWhereParams;
    playlistWhereParams[":id"] = id;
    
    bool success = m_dbService->remove(PLAYLISTS_TABLE, playlistWhereClause, playlistWhereParams);
    if (!success) {
        Log.error(QString("Failed to delete playlist %1").arg(id));
        return false;
    }
    
    Log.info(QString("Playlist %1 deleted successfully").arg(id));
    emit playlistDeleted(id);
    return true;
}

bool PlaylistManager::addMusicToPlaylist(int playlistId, const QString &musicHash, bool isLocal)
{
    // Dedup check: only active records (is_removed = 0)
    QueryOptions dupCheck;
    dupCheck.whereClause = "playlist_id = :pid AND music_hash = :hash AND is_removed = 0";
    dupCheck.whereParams = {{":pid", playlistId}, {":hash", musicHash}};
    auto existing = m_dbService->select(PLAYLIST_ITEMS_TABLE, dupCheck);
    if (!existing.isEmpty()) {
        Log.debug(QString("Music %1 already in playlist %2, skipping").arg(musicHash).arg(playlistId));
        QVariantMap updateLocal;
        updateLocal["is_local"] = isLocal ? 1 : 0;
        m_dbService->update(PLAYLIST_ITEMS_TABLE, updateLocal,
            "playlist_id = :pid AND music_hash = :hash",
            {{":pid", playlistId}, {":hash", musicHash}});
        return true;
    }

    // Check if deleted record exists for recovery
    QueryOptions delCheck;
    delCheck.whereClause = "playlist_id = :pid AND music_hash = :hash AND is_removed = 1";
    delCheck.whereParams = {{":pid", playlistId}, {":hash", musicHash}};
    auto deleted = m_dbService->select(PLAYLIST_ITEMS_TABLE, delCheck);
    if (!deleted.isEmpty()) {
        QVariantMap restoreData;
        restoreData["is_removed"] = 0;
        restoreData["is_local"] = isLocal ? 1 : 0;
        restoreData["updated_at"] = QDateTime::currentDateTime();
        m_dbService->update(PLAYLIST_ITEMS_TABLE, restoreData,
            "playlist_id = :pid AND music_hash = :hash",
            {{":pid", playlistId}, {":hash", musicHash}});
        emit musicAddedToPlaylist(playlistId, musicHash);
        if (playlistId == FAVORITE_PLAYLIST_ID) {
            emit favoriteStatusChanged(musicHash, true);
        }
        touchPlaylistTimestamp(playlistId);
        return true;
    }

    QueryOptions options;
    options.whereClause = "playlist_id = :playlist_id AND is_removed = 0 ORDER BY position DESC LIMIT 1";
    options.whereParams = { {":playlist_id", playlistId} };
    
    auto items = m_dbService->select(PLAYLIST_ITEMS_TABLE, options);
    int newPosition = items.isEmpty() ? 0 : items.first().value("position").toInt() + 1;
    
    QStringList columns = {"playlist_id", "music_hash", "position", "is_local", "updated_at"};
    QVariantMap values = {
        {"playlist_id", playlistId},
        {"music_hash", musicHash},
        {"position", newPosition},
        {"is_local", isLocal ? 1 : 0},
        {"updated_at", QDateTime::currentDateTime()}
    };
    
    int result = m_dbService->insert(PLAYLIST_ITEMS_TABLE, values);
    if (result >= 0) {
        emit musicAddedToPlaylist(playlistId, musicHash);
        if (playlistId == FAVORITE_PLAYLIST_ID) {
            emit favoriteStatusChanged(musicHash, true);
        }
        touchPlaylistTimestamp(playlistId);
        return true;
    }
    Log.warning(QString("[Playlist] addMusicToPlaylist failed: playlist=%1 hash=%2")
        .arg(playlistId).arg(musicHash));
    return false;
}

bool PlaylistManager::addMusicToPlaylistSync(int playlistId, const QString &musicHash)
{
    return addMusicToPlaylist(playlistId, musicHash);
}

bool PlaylistManager::addMusicToPlaylistSync(int playlistId, const QString &musicHash, bool isLocal)
{
    return addMusicToPlaylist(playlistId, musicHash, isLocal);
}

bool PlaylistManager::removeMusicFromPlaylist(int playlistId, const QString &musicHash)
{
    Log.info(QString("[Remove] playlist=%1 hash=%2 isFavorite=%3")
        .arg(playlistId).arg(musicHash.left(12)).arg(playlistId == FAVORITE_PLAYLIST_ID));
    
    if (playlistId < 0 || musicHash.isEmpty()) {
        Log.warning("Invalid playlistId or musicHash");
        return false;
    }

    bool success;
    {
        QVariantMap updateData;
        updateData["is_removed"] = 1;
        updateData["updated_at"] = QDateTime::currentDateTime();
        QVariantMap whereParams;
        whereParams[":playlist_id"] = playlistId;
        whereParams[":music_hash"] = musicHash;
        success = m_dbService->update(PLAYLIST_ITEMS_TABLE, updateData,
            "playlist_id = :playlist_id AND music_hash = :music_hash", whereParams);
    }
    Log.info(QString("[Remove] result=%1").arg(success));
    
    if (success) {
        Log.info(QString("Successfully removed music from playlist %1").arg(playlistId));
        emit musicRemovedFromPlaylist(playlistId, musicHash);
        if (playlistId == FAVORITE_PLAYLIST_ID) {
            emit favoriteStatusChanged(musicHash, false);
        }
        touchPlaylistTimestamp(playlistId);
    } else {
        Log.error(QString("Failed to remove music from playlist %1").arg(playlistId));
    }
    
    return success;
}

bool PlaylistManager::removeMusicFromPlaylistSync(int playlistId, const QString &musicHash)
{
    if (playlistId < 0 || musicHash.isEmpty()) return false;

    bool ok;
    {
        QVariantMap updateData;
        updateData["is_removed"] = 1;
        updateData["updated_at"] = QDateTime::currentDateTime();
        QVariantMap whereParams;
        whereParams[":playlist_id"] = playlistId;
        whereParams[":music_hash"] = musicHash;
        ok = m_dbService->update(PLAYLIST_ITEMS_TABLE, updateData,
            "playlist_id = :playlist_id AND music_hash = :music_hash", whereParams);
    }
    if (ok) touchPlaylistTimestamp(playlistId);
    return ok;
}

bool PlaylistManager::restoreMusicToPlaylist(int playlistId, const QString &musicHash)
{
    if (playlistId < 0 || musicHash.isEmpty()) return false;
    QVariantMap updateData;
    updateData["is_removed"] = 0;
    QVariantMap whereParams;
    whereParams[":playlist_id"] = playlistId;
    whereParams[":music_hash"] = musicHash;
    return m_dbService->update(PLAYLIST_ITEMS_TABLE, updateData,
        "playlist_id = :playlist_id AND music_hash = :music_hash", whereParams);
}

bool PlaylistManager::clearPlaylistItems(int playlistId)
{
    if (playlistId < 0) {
        Log.warning("clearPlaylistItems: invalid playlistId");
        return false;
    }

    // Logical delete: set is_removed = 1
    QVariantMap updateData;
    updateData["is_removed"] = 1;
    QVariantMap whereParams;
    whereParams[":playlist_id"] = playlistId;
    bool success = m_dbService->update(PLAYLIST_ITEMS_TABLE, updateData,
        "playlist_id = :playlist_id", whereParams);
    Log.info(QString("Cleared playlist %1 items").arg(playlistId));
    return success;
}

QVector<PlaylistItem> PlaylistManager::getPlaylistItems(int playlistId)
{
    QVector<PlaylistItem> result;
    QueryOptions options;
    options.whereClause = "playlist_id = :playlist_id AND is_removed = 0 ORDER BY position ASC";
    options.whereParams = { {":playlist_id", playlistId} };
    
    auto items = m_dbService->select(PLAYLIST_ITEMS_TABLE, options);
    
    for (const auto &item : items) {
        PlaylistItem pi;
        pi.id = item.value("id").toInt();
        pi.playlistId = item.value("playlist_id").toInt();
        pi.musicHash = item.value("music_hash").toString();
        pi.position = item.value("position").toInt();
        
        QString addedAtStr = item.value("added_at").toString();
        if (!addedAtStr.isEmpty()) {
            pi.addedAt = addedAtStr;
        }
        
        result.append(pi);
    }
    
    return result;
}

QVariantList PlaylistManager::getItems(int playlistId)
{
    QVariantList result;
    QueryOptions options;
    options.whereClause = "playlist_id = :playlist_id ORDER BY position ASC";
    options.whereParams = { {":playlist_id", playlistId} };
    
    auto items = m_dbService->select(PLAYLIST_ITEMS_TABLE, options);
    
    for (const auto &item : items) {
        QVariantMap map;
        map.insert("id", item.value("id").toInt());
        map.insert("playlistId", item.value("playlist_id").toInt());
        map.insert("musicHash", item.value("music_hash").toString());
        map.insert("position", item.value("position").toInt());
        
        QString addedAtStr = item.value("added_at").toString();
        if (!addedAtStr.isEmpty()) {
            map.insert("addedAt", addedAtStr);
        }
        
        result.append(map);
    }
    
    return result;
}

QVector<PlaylistMusicItem> PlaylistManager::getPlaylistMusicItems(int playlistId)
{
    QVector<PlaylistMusicItem> result;
    
    QString query = "SELECT pi.music_hash, pi.position, mf.title, mf.artist, mf.album, "
                    "mf.length_in_seconds, mf.file_path, mf.cover_hash "
                    "FROM playlist_items pi "
                    "LEFT JOIN music_files mf ON pi.music_hash = mf.file_hash "
                    "WHERE pi.playlist_id = :playlist_id AND pi.is_removed = 0 ORDER BY pi.position ASC";
    
    auto* sqliteService = qobject_cast<SqliteDatabaseService*>(m_dbService);
    if (!sqliteService) {
        Log.error("Failed to cast IDatabaseService to SqliteDatabaseService");
        return result;
    }
    QSqlQuery sqlQuery(sqliteService->getDatabase());
    sqlQuery.prepare(query);
    sqlQuery.bindValue(":playlist_id", playlistId);
    
    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            PlaylistMusicItem item;
            item.musicHash = sqlQuery.value("music_hash").toString();
            item.position = sqlQuery.value("position").toInt();
            item.title = sqlQuery.value("title").toString();
            item.artist = sqlQuery.value("artist").toString();
            item.album = sqlQuery.value("album").toString();
            
            int lengthInSeconds = sqlQuery.value("length_in_seconds").toInt();
            int minutes = lengthInSeconds / 60;
            int seconds = lengthInSeconds % 60;
            item.duration = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
            
            item.filePath = sqlQuery.value("file_path").toString();
            QString coverHash = sqlQuery.value("cover_hash").toString();
            if (!coverHash.isEmpty()) {
#ifdef Q_OS_ANDROID
                QString appDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
                QString appDir = QCoreApplication::applicationDirPath();
#endif
                item.coverPath = QString("file://%1/covers/%2/%3/%4.jpg")
                    .arg(appDir, coverHash.left(2), coverHash.mid(2, 2), coverHash);
            }
            
            result.append(item);
        }
    } else {
        Log.error("Failed to query playlist music items: " + sqlQuery.lastError().text());
    }

    Log.info(QString("[Sync] getPlaylistMusicItems id=%1 found=%2").arg(playlistId).arg(result.size()));
    return result;
}

Playlist PlaylistManager::getFavoritePlaylist()
{
    if (FAVORITE_PLAYLIST_ID > 0) {
        return getPlaylist(FAVORITE_PLAYLIST_ID);
    }
    return Playlist();
}

int PlaylistManager::getFavoritePlaylistId()
{
    return FAVORITE_PLAYLIST_ID;
}

bool PlaylistManager::isMusicFavorite(const QString &musicHash)
{
    return FavoriteManager::instance().isFavorite(musicHash);
}

QVariantList PlaylistManager::getFavoriteMusicHashes()
{
    QVariantList result;
    const auto& hashes = FavoriteManager::instance().favoriteHashes();
    for (const QString &hash : hashes) {
        result.append(hash);
    }
    return result;
}

QStringList PlaylistManager::takePendingDeletedUuids()
{
    QStringList result;
    for (const QString& uuid : m_pendingDeletedUuids) {
        result.append(uuid);
    }
    m_pendingDeletedUuids.clear();
    return result;
}

QList<QMap<QString, QVariant>> PlaylistManager::getPlaylistItemsRaw(int playlistId)
{
    QueryOptions options;
    options.whereClause = "playlist_id = :playlist_id ORDER BY position ASC";
    options.whereParams = {{":playlist_id", playlistId}};
    return m_dbService->select(PLAYLIST_ITEMS_TABLE, options);
}

SyncTask PlaylistManager::exportSyncData()
{
    QJsonObject root;
    root["version"] = 3;
    root["last_modified"] = QDateTime::currentSecsSinceEpoch();
    root["device_id"] = "";

    QJsonArray playlistsArr;

    const auto& normalPls = getAllPlaylists();
    for (const auto& pl : normalPls) {
        QJsonObject plObj;
        plObj["uuid"] = pl.uuid;
        plObj["name"] = pl.name;
        plObj["description"] = pl.description;
        plObj["is_collection"] = false;
        plObj["source_folder"] = pl.sourceFolder;
        plObj["created_at"] = pl.createdAt.toSecsSinceEpoch();
        plObj["updated_at"] = pl.updatedAt.toSecsSinceEpoch();

        QJsonArray itemsArr;
        const auto items = getPlaylistItemsRaw(pl.id);
        for (const auto& item : items) {
            QJsonObject itemObj;
            itemObj["music_hash"] = item.value("music_hash").toString();
            itemObj["position"] = item.value("position").toInt();
            itemObj["is_removed"] = item.value("is_removed").toInt();
            itemObj["is_local"] = item.value("is_local", 1).toInt();
            itemObj["updated_at"] = item.value("updated_at").toString();
            itemsArr.append(itemObj);
        }
        plObj["items"] = itemsArr;
        playlistsArr.append(plObj);
    }

    const auto& collectionPls = getCollectionPlaylists();
    for (const auto& pl : collectionPls) {
        QJsonObject plObj;
        plObj["uuid"] = pl.uuid;
        plObj["name"] = pl.name;
        plObj["description"] = pl.description;
        plObj["is_collection"] = true;
        plObj["source_folder"] = pl.sourceFolder;
        plObj["created_at"] = pl.createdAt.toSecsSinceEpoch();
        plObj["updated_at"] = pl.updatedAt.toSecsSinceEpoch();

        QJsonArray itemsArr;
        const auto items = getPlaylistItemsRaw(pl.id);
        for (const auto& item : items) {
            QJsonObject itemObj;
            itemObj["music_hash"] = item.value("music_hash").toString();
            itemObj["position"] = item.value("position").toInt();
            itemObj["is_removed"] = item.value("is_removed").toInt();
            itemObj["is_local"] = item.value("is_local", 1).toInt();
            itemObj["updated_at"] = item.value("updated_at").toString();
            itemsArr.append(itemObj);
        }
        plObj["items"] = itemsArr;
        playlistsArr.append(plObj);
        Log.info(QString("[Sync] export collection: name='%1' uuid=%2 items=%3")
            .arg(pl.name).arg(pl.uuid.left(12)).arg(itemsArr.size()));
    }

    Playlist favPl = getFavoritePlaylist();
    Log.info(QString("[Sync] export Favorites: found=%1 id=%2 createdAt=%3 updatedAt=%4 valid=%5")
        .arg(favPl.id >= 0 ? "yes" : "no")
        .arg(favPl.id)
        .arg(favPl.createdAt.toSecsSinceEpoch())
        .arg(favPl.updatedAt.toSecsSinceEpoch())
        .arg(favPl.updatedAt.isValid() ? "yes" : "no"));
    if (favPl.id >= 0) {
        QJsonObject plObj;
        plObj["uuid"] = favPl.uuid;
        plObj["name"] = favPl.name;
        plObj["description"] = favPl.description;
        plObj["is_collection"] = false;
        plObj["source_folder"] = "";
        plObj["created_at"] = favPl.createdAt.toSecsSinceEpoch();
        plObj["updated_at"] = favPl.updatedAt.toSecsSinceEpoch();

        QueryOptions titleOpt;
        titleOpt.setColumns({"file_hash", "title"});
        auto titleRows = m_dbService->select("music_files", titleOpt);
        QHash<QString, QString> titleMap;
        for (const auto& row : titleRows)
            titleMap[row["file_hash"].toString()] = row["title"].toString();

        QJsonArray itemsArr;
        const auto items = getPlaylistItemsRaw(favPl.id);
        for (const auto& item : items) {
            QJsonObject itemObj;
            QString h = item.value("music_hash").toString();
            itemObj["music_hash"] = h;
            itemObj["position"] = item.value("position").toInt();
            itemObj["favorite"] = true;
            itemObj["is_local"] = 1;
            itemObj["title"] = titleMap.value(h);
            itemObj["updated_at"] = item.value("updated_at").toString();
            itemsArr.append(itemObj);
        }
        plObj["items"] = itemsArr;
        playlistsArr.append(plObj);
    }

    root["playlists"] = playlistsArr;

    QJsonArray deletedArr;
    QStringList deletedUuids = takePendingDeletedUuids();
    qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const QString& uuid : deletedUuids) {
        QJsonObject delObj;
        delObj["uuid"] = uuid;
        delObj["deleted_at"] = now;
        deletedArr.append(delObj);
    }
    root["deleted"] = deletedArr;

    SyncTask task;
    task.section = sectionName();
    task.payload = root;
    task.lastModified = QDateTime::currentSecsSinceEpoch();
    return task;
}

SyncTask PlaylistManager::mergeSyncData(const SyncTask& local, const SyncTask& remote)
{
    QJsonObject localObj = local.payload.toObject();
    QJsonObject remoteObj = remote.payload.toObject();

    if (remoteObj.isEmpty()) {
        SyncTask t; t.section = sectionName(); t.payload = local.payload; return t;
    }
    if (localObj.isEmpty()) {
        SyncTask t; t.section = sectionName(); t.payload = remote.payload; return t;
    }

    QJsonArray localPls = localObj.value("playlists").toArray();
    QJsonArray remotePls = remoteObj.value("playlists").toArray();

    QMap<QString, QJsonObject> mergedMap;
    for (const auto& val : remotePls) {
        QJsonObject pl = val.toObject();
        QString uuid = pl.value("uuid").toString();
        if (!uuid.isEmpty()) mergedMap[uuid] = pl;
    }
    for (const auto& val : localPls) {
        QJsonObject pl = val.toObject();
        QString uuid = pl.value("uuid").toString();
        if (uuid.isEmpty()) continue;
        if (mergedMap.contains(uuid)) {
            QJsonArray localItems = pl.value("items").toArray();
            QJsonArray remoteItems = mergedMap[uuid].value("items").toArray();

            QMap<QString, QJsonObject> itemMap;
            for (const auto& v : remoteItems) {
                QJsonObject item = v.toObject();
                QString h = item.value("music_hash").toString();
                if (!h.isEmpty()) itemMap[h] = item;
            }
            for (const auto& v : localItems) {
                QJsonObject item = v.toObject();
                QString h = item.value("music_hash").toString();
                if (h.isEmpty()) continue;
                if (itemMap.contains(h)) {
                    QDateTime localItUpd = QDateTime::fromString(item.value("updated_at").toString(), Qt::ISODate);
                    QDateTime remoteItUpd = QDateTime::fromString(itemMap[h].value("updated_at").toString(), Qt::ISODate);
                    if (!remoteItUpd.isValid() || (localItUpd.isValid() && localItUpd >= remoteItUpd))
                        itemMap[h] = item;
                } else {
                    itemMap[h] = item;
                }
            }

            QJsonArray mergedItems;
            for (auto it = itemMap.constBegin(); it != itemMap.constEnd(); ++it)
                mergedItems.append(it.value());

            QJsonObject mergedPl = mergedMap[uuid];
            mergedPl["items"] = mergedItems;
            mergedMap[uuid] = mergedPl;

            Log.info(QString("[Sync] mergeSyncData uuid=%1: local=%2 remote=%3 merged=%4")
                .arg(uuid.left(8))
                .arg(localItems.size()).arg(remoteItems.size()).arg(mergedItems.size()));
        } else {
            mergedMap[uuid] = pl;
        }
    }

    QJsonArray mergedDeleted;
    QSet<QString> deletedUuids;
    auto collectDel = [&](const QJsonArray& arr) {
        for (const auto& v : arr) {
            QString u = v.toObject().value("uuid").toString();
            if (!u.isEmpty() && !deletedUuids.contains(u)) {
                deletedUuids.insert(u);
                mergedDeleted.append(v);
            }
        }
    };
    collectDel(localObj.value("deleted").toArray());
    collectDel(remoteObj.value("deleted").toArray());

    for (const QString& uuid : deletedUuids) {
        mergedMap.remove(uuid);
    }

    QJsonArray mergedPls;
    for (auto it = mergedMap.constBegin(); it != mergedMap.constEnd(); ++it) {
        mergedPls.append(it.value());
    }

    QJsonObject result;
    result["version"] = 3;
    result["last_modified"] = QDateTime::currentSecsSinceEpoch();
    result["playlists"] = mergedPls;
    result["deleted"] = mergedDeleted;

    SyncTask task;
    task.section = sectionName();
    task.payload = result;
    task.lastModified = QDateTime::currentSecsSinceEpoch();
    return task;
}

bool PlaylistManager::importSyncData(const SyncTask& task)
{
    QJsonObject merged = task.payload.toObject();
    Log.info(QString("[Sync] importSyncData: payload has 'playlists'=%1, 'deleted'=%2")
        .arg(merged.contains("playlists") ? "yes" : "no",
             merged.contains("deleted") ? "yes" : "no"));

    QJsonArray deletedArr = merged.value("deleted").toArray();
    for (const auto& delVal : deletedArr) {
        QString uuid = delVal.toObject().value("uuid").toString();
        if (uuid.isEmpty()) continue;
        Playlist pl = getPlaylistByUuid(uuid);
        if (pl.id >= 0) {
            deletePlaylist(pl.id);
            Log.info(QString("[DataSync] Deleted playlist: %1 (uuid=%2)").arg(pl.name, uuid));
        }
    }

    QJsonArray playlistsArr = merged.value("playlists").toArray();
    for (const auto& plVal : playlistsArr) {
        QJsonObject plObj = plVal.toObject();
        bool isFavorite = plObj.value("uuid").toString() == FAVORITE_PLAYLIST_UUID;

        if (isFavorite) {
            QJsonArray items = plObj.value("items").toArray();
            Log.info(QString("[Sync] Favorites import: total=%1 items in merged data")
                .arg(items.size()));

            if (!items.isEmpty()) {
                QJsonObject firstItem = items.first().toObject();
                Log.info(QString("[Sync] Favorites import: first item hash=%1 favorite=%2 pos=%3 title='%4'")
                    .arg(firstItem.value("music_hash").toString(),
                         QString::number(firstItem.value("favorite").toBool(true)),
                         QString::number(firstItem.value("position").toInt()),
                         firstItem.value("title").toString()));
            }

            int addedCount = 0;
            int removedCount = 0;
            for (const auto& itemVal : items) {
                QJsonObject itemObj = itemVal.toObject();
                QString musicHash = itemObj.value("music_hash").toString();
                if (musicHash.isEmpty()) continue;

                bool favorite = itemObj.value("favorite").toBool(true);
                if (favorite) {
                    addMusicToPlaylist(FAVORITE_PLAYLIST_ID, musicHash, true);
                    addedCount++;
                } else {
                    removeMusicFromPlaylist(FAVORITE_PLAYLIST_ID, musicHash);
                    removedCount++;
                }
            }
            touchPlaylistTimestamp(FAVORITE_PLAYLIST_ID);
            Log.info(QString("[Sync] Favorites import done: added=%1 removed=%2")
                .arg(addedCount).arg(removedCount));
        } else {
            QString uuid = plObj.value("uuid").toString();
            if (uuid.isEmpty()) { Log.warning("[DataSync] Playlist missing uuid, skipped"); continue; }

            QString name = plObj.value("name").toString();
            QString desc = plObj.value("description").toString();
            bool isCollection = plObj.value("is_collection").toBool(false);
            QString sourceFolder = plObj.value("source_folder").toString();

            Playlist localPl = getPlaylistByUuid(uuid);
            int playlistId = localPl.id;

            if (playlistId < 0) {
                playlistId = createPlaylistWithUuid(uuid, name, desc, isCollection, sourceFolder);
                Log.info(QString("[DataSync] Playlist created: %1").arg(name));
            } else {
                updatePlaylist(playlistId, name, desc);
                Log.info(QString("[DataSync] Playlist updated: %1").arg(name));
            }

            if (playlistId < 0) continue;

            QJsonArray items = plObj.value("items").toArray();
            Log.info(QString("[Sync] importSyncData playlist=%1 uuid=%2 name='%3' isCollection=%4 items=%5")
                .arg(playlistId)
                .arg(uuid.left(12))
                .arg(name)
                .arg(isCollection ? "yes" : "no")
                .arg(items.size()));

            if (items.size() > 0 && items.size() < 30) {
                for (int i = 0; i < items.size(); i++) {
                    auto itm = items[i].toObject();
                    Log.info(QString("[Sync]   item[%1] hash=%2 is_removed=%3 pos=%4")
                        .arg(i)
                        .arg(itm.value("music_hash").toString(),
                             QString::number(itm.value("is_removed").toInt()),
                             QString::number(itm.value("position").toInt())));
                }
            }

            auto localItems = getPlaylistItemsRaw(playlistId);
            QSet<QString> localActiveHashes;
            for (const auto& item : localItems) {
                QString h = item.value("music_hash").toString();
                if (!h.isEmpty() && item.value("is_removed").toInt() == 0)
                    localActiveHashes.insert(h);
            }

            for (const auto& itemVal : items) {
                QJsonObject itemObj = itemVal.toObject();
                QString musicHash = itemObj.value("music_hash").toString();
                if (musicHash.isEmpty()) continue;

                bool isDeleted = itemObj.value("is_removed").toInt(0) != 0;

                if (isDeleted) {
                    if (localActiveHashes.contains(musicHash)) {
                        removeMusicFromPlaylistSync(playlistId, musicHash);
                    }
                } else {
                    bool itemIsLocal = itemObj.value("is_local").toInt(1) != 0;
                    addMusicToPlaylistSync(playlistId, musicHash, itemIsLocal);
                    if (isCollection) {
                        Log.info(QString("[Sync] collection playlist=%1 hash=%2 is_local=%3")
                            .arg(playlistId).arg(musicHash).arg(itemIsLocal));
                    }
                    localActiveHashes.remove(musicHash);
                }
            }

            for (const QString& hash : localActiveHashes) {
                removeMusicFromPlaylistSync(playlistId, hash);
            }

            Log.info(QString("[DataSync] Playlist synced: %1 (id=%2)").arg(name).arg(playlistId));
        }
    }
    return true;
}

#include "metadataoverridemanager.h"
#include "../../interfaces/idatabaseservice.h"
#include "../../utils/logger.h"
#include "../../utils/query_options.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

MetadataOverrideManager::MetadataOverrideManager(QObject* parent, IDatabaseService* db)
    : QObject(parent), m_db(db) {}

bool MetadataOverrideManager::initialize()
{
    if (!m_db) {
        Log.error("MetadataOverrideManager: db is null");
        return false;
    }
    if (!initializeTable()) return false;
    loadAllOverridesToCache();
    return true;
}

bool MetadataOverrideManager::initializeTable()
{
    const QStringList schema = {
        "file_hash CHAR(40) PRIMARY KEY",
        "title TEXT",
        "artist TEXT",
        "album TEXT",
        "album_artist TEXT",
        "genre TEXT",
        "year INTEGER",
        "comment TEXT",
        "lyrics TEXT",
        "cover_hash CHAR(32)",
        "updated_at INTEGER DEFAULT (strftime('%s', 'now'))",
        "FOREIGN KEY (file_hash) REFERENCES music_files(file_hash) ON DELETE CASCADE"
    };

    if (!m_db->tableExists(TABLE_NAME)) {
        if (!m_db->createTable(TABLE_NAME, schema)) {
            Log.error("Failed to create " + TABLE_NAME + " table: " + m_db->lastError());
            return false;
        }
        Log.info(TABLE_NAME + " table created");
    }
    return true;
}

void MetadataOverrideManager::loadAllOverridesToCache()
{
    m_cache.clear();

    if (m_db->isTableEmpty(TABLE_NAME)) {
        Log.debug("metadata_overrides table is empty");
        return;
    }

    QueryOptions opt;
    opt.setColumns({"file_hash", "title", "artist", "album", "album_artist",
                    "genre", "year", "comment", "lyrics", "cover_hash"});
    auto rows = m_db->select(TABLE_NAME, opt);

    for (const auto& row : rows) {
        QString hash = row["file_hash"].toString();
        if (hash.isEmpty()) continue;

        QVariantMap clean;
        for (auto it = row.begin(); it != row.end(); ++it) {
            if (it.key() != "file_hash" && !it.value().isNull()) {
                clean[it.key()] = it.value();
            }
        }
        if (!clean.isEmpty()) {
            m_cache[hash] = clean;
        }
    }

    Log.info(QString("Loaded %1 metadata overrides to cache").arg(m_cache.size()));
}

void MetadataOverrideManager::setMetadata(const QString& fileHash, const QVariantMap& data)
{
    if (fileHash.isEmpty()) return;

    QVariantMap clean;
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.value().metaType().id() == QMetaType::QString && it.value().toString().isEmpty()) {
            clean[it.key()] = QVariant();
        } else {
            clean[it.key()] = it.value();
        }
    }

    QVariantMap whereParams;
    whereParams["file_hash"] = fileHash;
    bool updated = m_db->update(TABLE_NAME, clean, "file_hash = :file_hash", whereParams);

    if (!updated) {
        clean["file_hash"] = fileHash;
        m_db->insertIgnore(TABLE_NAME, clean);
    }

    if (m_cache.contains(fileHash)) {
        QVariantMap& existing = m_cache[fileHash];
        for (auto it = clean.begin(); it != clean.end(); ++it) {
            if (it.value().isNull()) {
                existing.remove(it.key());
            } else {
                existing[it.key()] = it.value();
            }
        }
    } else {
        m_cache[fileHash] = clean;
    }
}

void MetadataOverrideManager::setCoverHash(const QString& fileHash, const QString& coverHash)
{
    if (fileHash.isEmpty()) return;

    QVariantMap data;
    data["cover_hash"] = coverHash.isEmpty() ? QVariant() : QVariant(coverHash);
    setMetadata(fileHash, data);
}

QVariantMap MetadataOverrideManager::getOverrides(const QString& fileHash) const
{
    return m_cache.value(fileHash);
}

bool MetadataOverrideManager::hasOverrides(const QString& fileHash) const
{
    return m_cache.contains(fileHash) && !m_cache[fileHash].isEmpty();
}

QHash<QString, QVariantMap> MetadataOverrideManager::getAllOverrides() const
{
    return m_cache;
}

void MetadataOverrideManager::removeOverride(const QString& fileHash)
{
    QVariantMap where;
    where["file_hash"] = fileHash;
    m_db->remove(TABLE_NAME, "file_hash = :file_hash", where);
    m_cache.remove(fileHash);
}

SyncTask MetadataOverrideManager::exportSyncData()
{
    QJsonObject root;
    root["version"] = 2;
    root["last_modified"] = QDateTime::currentSecsSinceEpoch();

    QJsonObject overridesObj;
    for (auto it = m_cache.constBegin(); it != m_cache.constEnd(); ++it) {
        QJsonObject entry;
        const QVariantMap& data = it.value();
        if (data.contains("title"))        entry["title"]        = data["title"].toString();
        if (data.contains("artist"))       entry["artist"]       = data["artist"].toString();
        if (data.contains("album"))        entry["album"]        = data["album"].toString();
        if (data.contains("album_artist")) entry["album_artist"] = data["album_artist"].toString();
        if (data.contains("genre"))        entry["genre"]        = data["genre"].toString();
        if (data.contains("year"))         entry["year"]         = data["year"].toInt();
        if (data.contains("comment"))      entry["comment"]      = data["comment"].toString();
        if (data.contains("lyrics"))       entry["lyrics"]       = data["lyrics"].toString();
        if (data.contains("cover_hash"))   entry["cover_hash"]   = data["cover_hash"].toString();
        overridesObj[it.key()] = entry;
    }
    root["overrides"] = overridesObj;

    SyncTask task;
    task.section = sectionName();
    task.payload = root;
    task.lastModified = QDateTime::currentSecsSinceEpoch();
    return task;
}

SyncTask MetadataOverrideManager::mergeSyncData(const SyncTask& local, const SyncTask& remote)
{
    QJsonObject localObj = local.payload.toObject();
    QJsonObject remoteObj = remote.payload.toObject();

    if (remoteObj.isEmpty()) {
        SyncTask t; t.section = sectionName(); t.payload = local.payload; return t;
    }
    if (localObj.isEmpty()) {
        SyncTask t; t.section = sectionName(); t.payload = remote.payload; return t;
    }

    QJsonObject localOverrides = localObj.value("overrides").toObject();
    QJsonObject remoteOverrides = remoteObj.value("overrides").toObject();

    QSet<QString> allHashes;
    for (const auto& key : localOverrides.keys()) allHashes.insert(key);
    for (const auto& key : remoteOverrides.keys()) allHashes.insert(key);

    QJsonObject merged;
    for (const QString& hash : allHashes) {
        bool inLocal = localOverrides.contains(hash);
        bool inRemote = remoteOverrides.contains(hash);

        if (inLocal && !inRemote)
            merged[hash] = localOverrides[hash];
        else if (!inLocal && inRemote)
            merged[hash] = remoteOverrides[hash];
        else {
            qint64 localTime = static_cast<qint64>(localObj.value("last_modified").toDouble());
            qint64 remoteTime = static_cast<qint64>(remoteObj.value("last_modified").toDouble());
            merged[hash] = (localTime >= remoteTime)
                ? localOverrides[hash] : remoteOverrides[hash];
        }
    }

    QJsonObject result;
    result["version"] = 2;
    result["last_modified"] = QDateTime::currentSecsSinceEpoch();
    result["overrides"] = merged;

    SyncTask task;
    task.section = sectionName();
    task.payload = result;
    task.lastModified = QDateTime::currentSecsSinceEpoch();
    return task;
}

bool MetadataOverrideManager::importSyncData(const SyncTask& task)
{
    QJsonObject overridesObj = task.payload.toObject().value("overrides").toObject();
    for (auto it = overridesObj.constBegin(); it != overridesObj.constEnd(); ++it) {
        QJsonObject entry = it.value().toObject();
        QVariantMap data;
        if (entry.contains("title"))        data["title"]        = entry["title"].toString();
        if (entry.contains("artist"))       data["artist"]       = entry["artist"].toString();
        if (entry.contains("album"))        data["album"]        = entry["album"].toString();
        if (entry.contains("album_artist")) data["album_artist"] = entry["album_artist"].toString();
        if (entry.contains("genre"))        data["genre"]        = entry["genre"].toString();
        if (entry.contains("year"))         data["year"]         = entry["year"].toInt();
        if (entry.contains("comment"))      data["comment"]      = entry["comment"].toString();
        if (entry.contains("lyrics"))       data["lyrics"]       = entry["lyrics"].toString();
        if (entry.contains("cover_hash"))   data["cover_hash"]   = entry["cover_hash"].toString();

        setMetadata(it.key(), data);
        if (entry.contains("cover_hash"))
            setCoverHash(it.key(), entry["cover_hash"].toString());
    }
    return true;
}

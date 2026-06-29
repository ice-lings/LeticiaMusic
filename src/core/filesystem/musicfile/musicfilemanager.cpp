#include "musicfilemanager.h"

#include <QFileInfo>
#include <QTimer>
#include <QThread>

#include "../../interfaces/idatabaseservice.h"
#include "../../database/sqlite/sqlitedatabaseservice.h"
#include "../../utils/musicinfo.h"

#include "../../utils/logger.h"

MusicFileManager::MusicFileManager(QObject *parent, IDatabaseService *dbService)
    : QObject{parent}, m_databaseService(dbService)
{
    Q_ASSERT(m_databaseService);
}

bool MusicFileManager::initialize()
{
    Log.info(QString("[MFM Init] table=%1, initializing...").arg(TABLE_NAME));
    bool table_init = initializeTable();

    if (table_init && m_databaseService)
    {
        if (m_databaseService->isTableEmpty(TABLE_NAME))
        {
            Log.debug(TABLE_NAME + " is Empty!!! need to scan");
        }
    }

    return table_init;
}

void MusicFileManager::trySaveMusicFileInfoToDB(const MusicInfo &musicInfo, MusicItem &item)
{
    MusicFileInfo info;

    info.file_path = musicInfo.musicFilePath;
    QFileInfo fi(info.file_path);
    if (fi.exists()) {
        info.file_size = fi.size();
        info.last_modified = fi.lastModified().toSecsSinceEpoch();
    }
    info.file_hash = musicInfo.musicID;
    item.fileHash = info.file_hash;

    info.cover_hash = musicInfo.coverHash;

    info.title = item.title;
    info.artist = item.artist;
    info.album = item.album;

    info.album_artist = musicInfo.metadata.album_artist;
    info.genre = musicInfo.metadata.genre;
    info.year = musicInfo.metadata.year;
    info.length_in_seconds = musicInfo.metadata.lengthInSeconds;
    info.bitrate = musicInfo.metadata.bitrate;
    info.sample_rate = musicInfo.metadata.sampleRate;
    info.channels = musicInfo.metadata.channels;

    info.bpm = 0;
    info.comment = "";
    info.composer = "";
    info.lyrics = "";

    info.quality = classifyQuality(musicInfo.metadata.audioFormat,
                                   musicInfo.metadata.bitrate,
                                   musicInfo.metadata.sampleRate);
    item.quality = info.quality;

    info.metadata_extracted = true;

    if (info.isValid()) {
        QVariantMap insertData = info.toVariantMap();
        int result = m_databaseService->insertIgnore(TABLE_NAME, insertData);
        if (result >= 0) {
            Log.info(QString("--Music-- Insert success! %1 - %2").arg(info.artist, info.title));
        } else if (result == -2) {
            // 检查是否存在 is_deleted=1 的同 hash 记录，若存在则恢复
            if (restoreDeletedRecord(info)) {
                Log.info(QString("--Music-- Restored deleted record: %1 - %2").arg(info.artist, info.title));
            } else {
                Log.info(QString("--Music-- Already exists, skipped: %1").arg(info.file_path));
            }
        } else {
            Log.error(QString("--Music-- Insert failed! %1").arg(info.file_path));
        }
    } else {
        Log.warning(QString("--Music-- Music info invalid! %1").arg(info.file_path));
    }
}



bool MusicFileManager::initializeTable()
{
    const QStringList musicFilesSchema = {
    // 核心：音乐文件内容哈希（SHA1 40位），主键+非空，天然去重
    "file_hash CHAR(40) PRIMARY KEY NOT NULL",
    // 音乐文件基础属性：唯一路径、大小、最后修改时间（原有）
    "file_path TEXT NOT NULL UNIQUE",
    "file_size INTEGER NOT NULL",
    "last_modified INTEGER NOT NULL",
    // ====== 新增：音频基础属性（必加，TagLib提取，核心展示）======
    "length_in_seconds INTEGER DEFAULT 0", // 时长（秒），0表示未提取
    "bitrate INTEGER DEFAULT 0",            // 码率（kbps），0表示未提取
    "sample_rate INTEGER DEFAULT 0",        // 采样率（Hz），0表示未提取
    "channels INTEGER DEFAULT 0",           // 声道数（1=单声道，2=立体声），0表示未提取
    // 提取状态标识（原有）
    "metadata_extracted BOOLEAN DEFAULT 0",
    "cover_extracted BOOLEAN DEFAULT 0",
    // 封面关联字段（原有）
    "cover_hash CHAR(16) DEFAULT NULL",
    // 音乐核心元数据（原有）
    "title TEXT NOT NULL",
    "artist TEXT NOT NULL",
    "album TEXT DEFAULT NULL",
    "album_artist TEXT DEFAULT NULL",
    "genre TEXT DEFAULT NULL",
    "year INTEGER DEFAULT NULL",
    // ====== 新增：核心元数据（推荐/必加，作曲+歌词）======
    "composer TEXT DEFAULT NULL",           // 作曲人，NULL表示无/未提取
    "lyrics TEXT DEFAULT NULL",             // 歌词内容（核心，后续拓展），TEXT适配长文本
    // ====== 新增：低频元数据（可选，评论+BPM）======
    "comment TEXT DEFAULT NULL",            // 内嵌评论，NULL表示无/未提取
    "bpm INTEGER DEFAULT 0",                // 节拍数，0表示无/未提取
    "quality TEXT DEFAULT ''",              // 音质等级标签
    "audio_format TEXT DEFAULT ''",         // 真实音频格式（TagLib魔术字节检测）
    "is_deleted INTEGER DEFAULT 0",       // 1=已删除(逻辑删除), 0=正常

    "created_at INTEGER DEFAULT (strftime('%s', 'now'))",
    "updated_at INTEGER DEFAULT (strftime('%s', 'now'))"
    };


    if (!m_databaseService->tableExists(TABLE_NAME)) {
        if (!m_databaseService->createTable(TABLE_NAME, musicFilesSchema)) {
            Log.error("Failed to create music files table: " + m_databaseService->lastError());
            return false;
        }
        Log.info("Music files table created successfully");
    } else {
        Log.debug("Music files table already exists");
    }
    m_databaseService->executeRaw(
        QString("CREATE INDEX IF NOT EXISTS idx_music_title_artist ON %1(title, artist)").arg(TABLE_NAME));

    return true;
}

QSet<QString> MusicFileManager::getAllMusicIds() const {
    QSet<QString> ids;
    QueryOptions opt;
    opt.setColumns({"file_hash"});
    opt.setWhere("is_deleted = 0");
    auto result = m_databaseService->select(TABLE_NAME, opt);
    for (const auto& row : result) {
        ids.insert(row["file_hash"].toString());
    }
    return ids;
}

QHash<QString, QVariantMap> MusicFileManager::getBatchMusicMetadata() const
{
    QHash<QString, QVariantMap> result;
    QueryOptions opt;
    opt.setColumns({"file_hash", "title", "artist", "album",
                    "length_in_seconds", "bitrate", "sample_rate",
                    "channels", "audio_format", "cover_hash", "quality"});
    opt.setWhere("is_deleted = 0");
    auto rows = m_databaseService->select(TABLE_NAME, opt);
    for (const auto& row : rows) {
        QVariantMap m;
        m["title"]      = row["title"];
        m["artist"]     = row["artist"];
        m["album"]      = row["album"];
        m["length_in_seconds"] = row["length_in_seconds"];
        m["bitrate"]    = row["bitrate"];
        m["sample_rate"] = row["sample_rate"];
        m["channels"]   = row["channels"];
        m["audio_format"] = row["audio_format"];
        m["cover_hash"] = row["cover_hash"];
        m["quality"]    = row["quality"];
        result[row["file_hash"].toString()] = m;
    }
    return result;
}

QString MusicFileManager::calculateMusicFileHash(const QFileInfo &fileInfo)
{
    // 1. 基础校验：文件不存在/非普通文件（如目录/快捷方式），直接返回空
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return "";
    }

    // 2. 打开文件：只读+二进制模式（保证读取原始二进制内容，哈希计算精准）
    QFile musicFile(fileInfo.absoluteFilePath());
    if (!musicFile.open(QIODevice::ReadOnly)) {
        return "";
    }

    // 3. 初始化SHA1哈希计算器（固定算法，匹配数据库CHAR(40)主键）
    QCryptographicHash hashCalculator(QCryptographicHash::Sha1);
    // 分段读取缓冲区：4096字节（最优值，兼顾读取效率和内存占用，适配GB级无损音乐）
    const qint64 bufferSize = 4096;
    QByteArray readBuffer;

    // 4. 分段读取文件内容并更新哈希（核心：遍历所有二进制内容，无内存溢出）
    while (!musicFile.atEnd()) {
        readBuffer = musicFile.read(bufferSize);
        // 读取失败判定：缓冲区为空且未到文件末尾，说明文件读取异常
        if (readBuffer.isEmpty() && !musicFile.atEnd()) {
            musicFile.close(); // 释放文件句柄
            return "";
        }
        hashCalculator.addData(readBuffer);
    }

    // 5. 必须关闭文件：释放文件句柄，避免文件被占用
    musicFile.close();

    // 6. 计算最终哈希，转**纯小写十六进制字符串**返回（40位，直接作为数据库主键）
    QByteArray finalHash = hashCalculator.result();
    return finalHash.toHex().toLower();
}

QString MusicFileManager::calculateMusicFileHash(const QString &absoluteFilePath)
{

    if (absoluteFilePath.isEmpty()) {
        return "";
    }
    return calculateMusicFileHash(QFileInfo(absoluteFilePath));
}

MusicItem MusicFileManager::getMusicItemByFilePath(const QString& filePath) const
{
    if (filePath.isEmpty()) {
        return MusicItem{};
    }

    const QString absSearch = QFileInfo(filePath).absoluteFilePath();
    const auto& list = getMusicList();
    for (const auto& item : list) {
        if (QFileInfo(item.filePath).absoluteFilePath() == absSearch) {
            return item;
        }
    }

    return MusicItem{};
}

MusicItem MusicFileManager::getMusicItemByFileHash(const QString& fileHash) const
{
    if (fileHash.isEmpty()) {
        return MusicItem{};
    }

    const auto& list = getMusicList();
    for (const auto& item : list) {
        if (item.fileHash == fileHash) {
            return item;
        }
    }

    return MusicItem{};
}

int MusicFileManager::countByCoverHash(const QString& coverHash) const
{
    if (coverHash.isEmpty() || !m_databaseService) return 0;

    QueryOptions opt;
    opt.setColumns({"file_hash"});
    opt.setWhere("cover_hash = :cover_hash", {{"cover_hash", coverHash}});
    return m_databaseService->select(TABLE_NAME, opt).size();
}

int MusicFileManager::countCoverRefsAcrossTables(const QString& coverHash,
                                                   const QString& excludeFileHash) const
{
    if (coverHash.isEmpty() || !m_databaseService) return 0;

    QueryOptions opt1;
    opt1.setColumns({"file_hash"});
    if (!excludeFileHash.isEmpty())
        opt1.setWhere("cover_hash = :hash AND file_hash != :exclude",
                      {{"hash", coverHash}, {"exclude", excludeFileHash}});
    else
        opt1.setWhere("cover_hash = :hash", {{"hash", coverHash}});
    int count1 = m_databaseService->select(TABLE_NAME, opt1).size();

    QueryOptions opt2;
    opt2.setColumns({"file_hash"});
    if (!excludeFileHash.isEmpty())
        opt2.setWhere("cover_hash = :hash AND file_hash != :exclude",
                      {{"hash", coverHash}, {"exclude", excludeFileHash}});
    else
        opt2.setWhere("cover_hash = :hash", {{"hash", coverHash}});
    int count2 = m_databaseService->select("metadata_overrides", opt2).size();

    return count1 + count2;
}

bool MusicFileManager::deleteMusicByHash(const QString& fileHash)
{
    if (fileHash.isEmpty() || !m_databaseService) return false;

    QVariantMap updateData;
    updateData["is_deleted"] = 1;
    updateData["updated_at"] = QDateTime::currentSecsSinceEpoch();
    QVariantMap whereParams;
    whereParams["file_hash"] = fileHash;
    bool dbResult = m_databaseService->update(TABLE_NAME, updateData,
        "file_hash = :file_hash", whereParams);

    if (dbResult) {
        auto& list = m_musicList;
        for (int i = 0; i < list.size(); ++i) {
            if (list.at(i).fileHash == fileHash) {
                list.removeAt(i);
                break;
            }
        }
    }

    return dbResult;
}

bool MusicFileManager::restoreMusicByHash(const QString& fileHash)
{
    if (fileHash.isEmpty() || !m_databaseService) return false;

    QVariantMap updateData;
    updateData["is_deleted"] = 0;
    updateData["updated_at"] = QDateTime::currentSecsSinceEpoch();
    QVariantMap whereParams;
    whereParams["file_hash"] = fileHash;
    bool dbResult = m_databaseService->update(TABLE_NAME, updateData,
        "file_hash = :file_hash", whereParams);

    if (dbResult) {
        MusicItem item = loadMusicItemByHash(fileHash);
        if (!item.fileHash.isEmpty()) {
            m_musicList.append(item);
        }
    }

    return dbResult;
}

bool MusicFileManager::restoreDeletedRecord(const MusicFileInfo& info)
{
    if (!m_databaseService) return false;

    // 检查是否存在 is_deleted=1 的同 hash 记录
    QueryOptions opt;
    opt.setColumns({"file_hash"});
    opt.setWhere("file_hash = :hash AND is_deleted = 1", {{"hash", info.file_hash}});
    auto results = m_databaseService->select(TABLE_NAME, opt);
    if (results.isEmpty()) return false;

    QVariantMap updateData;
    updateData["is_deleted"] = 0;
    updateData["file_path"] = info.file_path;
    updateData["file_size"] = info.file_size;
    updateData["last_modified"] = info.last_modified;
    updateData["updated_at"] = QDateTime::currentSecsSinceEpoch();
    QVariantMap whereParams;
    whereParams["file_hash"] = info.file_hash;
    return m_databaseService->update(TABLE_NAME, updateData,
        "file_hash = :file_hash", whereParams);
}

QString MusicFileManager::findExistingFileByHash(const QString& hash) const
{
    if (hash.isEmpty() || !m_databaseService) return QString();

    QueryOptions opt;
    opt.setColumns({"file_path"});
    opt.setWhere("file_hash = :hash AND is_deleted = 0", {{"hash", hash}});
    auto results = m_databaseService->select(TABLE_NAME, opt);
    if (results.isEmpty()) return QString();

    QString path = results.first()["file_path"].toString();
    if (QFile::exists(path)) return path;

    return QString();
}

int MusicFileManager::getDeletedMusicCount() const
{
    if (!m_databaseService) return 0;
    QueryOptions opt;
    opt.setColumns({"file_hash"});
    opt.setWhere("is_deleted = 1");
    return m_databaseService->select(TABLE_NAME, opt).size();
}

QVariantList MusicFileManager::getDeletedMusicList() const
{
    QVariantList list;
    if (!m_databaseService) return list;

    QueryOptions opt;
    opt.setColumns({"file_hash", "file_path", "title", "updated_at"});
    opt.setWhere("is_deleted = 1");
    auto rows = m_databaseService->select(TABLE_NAME, opt);

    for (const auto& row : rows) {
        QVariantMap m;
        m["title"] = row["title"];
        m["fileHash"] = row["file_hash"];
        m["filePath"] = row["file_path"];
        m["deletedAt"] = row["updated_at"];
        m["status"] = QStringLiteral("pending");
        m["fileExists"] = !row["file_path"].toString().isEmpty()
                          && QFile::exists(row["file_path"].toString());
        list.append(m);
    }
    return list;
}

QVariantMap MusicFileManager::getMusicFileRecord(const QString& fileHash) const
{
    if (fileHash.isEmpty() || !m_databaseService) return {};

    QueryOptions opt;
    opt.setColumns({"file_hash", "file_path", "title", "artist", "album",
                    "length_in_seconds", "cover_hash", "quality",
                    "is_deleted", "updated_at"});
    opt.setWhere("file_hash = :hash", {{":hash", fileHash}});
    auto results = m_databaseService->select(TABLE_NAME, opt);
    if (results.isEmpty()) return {};
    return results.first();
}

bool MusicFileManager::permanentlyRemoveMusic(const QString& fileHash)
{
    if (fileHash.isEmpty() || !m_databaseService) return false;

    m_databaseService->executeRaw(
        QString("DELETE FROM playlist_items WHERE music_hash = '%1'").arg(fileHash));

    m_databaseService->executeRaw(
        QString("DELETE FROM %1 WHERE file_hash = '%2'").arg(TABLE_NAME, fileHash));

    auto& list = m_musicList;
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).fileHash == fileHash) {
            list.removeAt(i);
            break;
        }
    }
    return true;
}

MusicItem MusicFileManager::loadMusicItemByHash(const QString& hash) const
{
    MusicItem item;
    if (hash.isEmpty() || !m_databaseService) return item;

    QueryOptions opt;
    opt.setColumns({"file_hash", "file_path", "title", "artist", "album", "length_in_seconds", "cover_hash", "quality"});
    opt.setWhere("file_hash = :hash AND is_deleted = 0", {{":hash", hash}});
    auto results = m_databaseService->select(TABLE_NAME, opt);
    if (results.isEmpty()) return item;

    const auto& row = results.first();
    item.fileHash = row["file_hash"].toString();
    item.filePath = row["file_path"].toString();
    item.title = row["title"].toString();
    item.artist = row["artist"].toString();
    item.album = row["album"].toString();
    item.duration = formatDuration(row["length_in_seconds"].toInt());
    item.coverHash = row["cover_hash"].toString();
    item.quality = row["quality"].toString();

    return item;
}

QList<QVariantMap> MusicFileManager::queryByTitleArtist(const QString& title, const QString& artist) const
{
    QList<QVariantMap> result;
    if (title.isEmpty() || artist.isEmpty() || !m_databaseService) return result;

    QueryOptions opt;
    opt.setColumns({"file_hash", "file_path", "title", "artist", "album",
                    "length_in_seconds", "bitrate", "sample_rate",
                    "cover_hash", "quality", "audio_format"});
    opt.setWhere("title = :title AND artist = :artist",
                 {{":title", title}, {":artist", artist}});
    auto rows = m_databaseService->select(TABLE_NAME, opt);

    for (const auto& row : rows) {
        QVariantMap map;
        map["fileHash"] = row["file_hash"];
        map["filePath"] = row["file_path"];
        map["title"] = row["title"];
        map["artist"] = row["artist"];
        map["album"] = row["album"];
        map["lengthInSeconds"] = row["length_in_seconds"];
        map["bitrate"] = row["bitrate"];
        map["sampleRate"] = row["sample_rate"];
        map["coverHash"] = row["cover_hash"];
        map["quality"] = row["quality"];
        map["audioFormat"] = row["audio_format"];
        result.append(map);
    }
    return result;
}

void MusicFileManager::refreshMusicList()
{
    m_musicList.clear();

    auto* sqliteSvc = qobject_cast<SqliteDatabaseService*>(m_databaseService);
    if (!sqliteSvc) return;

    QSqlQuery query(sqliteSvc->getDatabase());
    QString sql = "SELECT DISTINCT mf.file_hash, mf.file_path, mf.title, mf.artist, mf.album, "
                  "mf.length_in_seconds, mf.cover_hash, mf.quality "
                  "FROM music_files mf "
                  "LEFT JOIN playlist_items pi ON pi.music_hash = mf.file_hash AND pi.is_removed = 0 "
                  "WHERE mf.is_deleted = 0 "
                  "  AND (pi.playlist_id IS NULL OR pi.is_local = 1)";
    query.prepare(sql);
    if (query.exec()) {
        while (query.next()) {
            MusicItem item;
            item.fileHash = query.value("file_hash").toString();
            item.filePath = query.value("file_path").toString();
            item.title = query.value("title").toString();
            item.artist = query.value("artist").toString();
            item.album = query.value("album").toString();
            item.duration = formatDuration(query.value("length_in_seconds").toInt());
            item.coverHash = query.value("cover_hash").toString();
            item.quality = query.value("quality").toString();
            m_musicList.append(item);
        }
    }
    return;
}

void MusicFileManager::startAsyncValidation()
{
    if (m_musicList.isEmpty()) return;

    Log.debug(QString("[MusicFileManager] 开始异步校验 %1 个音乐文件...").arg(m_musicList.size()));

    auto musicList = m_musicList;

    QThread::create([this, musicList]() {
        QSet<QString> missingHashes;
        for (const auto& item : musicList) {
            if (!QFile::exists(item.filePath)) {
                missingHashes.insert(item.fileHash);
            }
        }

        QMetaObject::invokeMethod(this, [this, missingHashes]() {
            if (!missingHashes.isEmpty()) {
                Log.info(QString("[MusicFileManager] 文件校验完成，缺失 %1 个，将通知 ViewModel 标记").arg(missingHashes.size()));
                emit validationComplete(missingHashes);
            } else {
                Log.debug("[MusicFileManager] 文件校验完成，所有文件完整");
            }
        }, Qt::QueuedConnection);
    })->start();
}

QString MusicFileManager::classifyQuality(const QString& audioFormat, int bitrate, int sampleRate)
{
    // 无损格式集合（通过TagLib魔术字节判定，与文件后缀无关）
    static const QSet<QString> losslessFormats = {
        "FLAC", "WAV", "APE", "WavPack", "TTA", "AIFF", "OggFLAC"
    };

    if (losslessFormats.contains(audioFormat)) {
        if (sampleRate >= 96000) return "HI";
        return "SQ";
    }

    // M4A容器：AAC(有损) 或 ALAC(无损)，通过码率区分 — ALAC ≥500kbps
    if (audioFormat == "M4A") {
        if (bitrate >= 500) {
            if (sampleRate >= 96000) return "HI";
            return "SQ";
        }
    }

    // 有损格式：按码率分档
    if (bitrate >= 256) return "HQ";
    if (bitrate >= 128) return "SD";
    if (bitrate > 0)   return "LQ";

    return ""; // 无法判定，不显示
}

QString MusicFileManager::qualityColor(const QString& qualityLabel)
{
    if (qualityLabel == "HI") return "#E5C07B";
    if (qualityLabel == "SQ") return "#C678DD";
    if (qualityLabel == "HQ") return "#61AFEF";
    if (qualityLabel == "SD") return "#98C379";
    return "transparent"; // LQ 或空
}


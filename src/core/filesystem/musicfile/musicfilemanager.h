#ifndef MUSICFILEMANAGER_H
#define MUSICFILEMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QFileInfo>
#include <QSet>

#include "../../utils/musicinfo.h"


class MusicFileManager : public QObject
{
    Q_OBJECT
public:
    explicit MusicFileManager(QObject* parent = nullptr, class IDatabaseService* dbService = nullptr);


    bool initialize();

    void trySaveMusicFileInfoToDB(const MusicInfo& FilePath, class MusicItem& item);

    MusicItem loadMusicItemByHash(const QString& hash) const;
    void refreshMusicList();


    struct MusicFileInfo
    {
        // ===================== 核心主键（原有）=====================
        QString file_hash;       // CHAR(40) PRIMARY KEY NOT NULL

        // ===================== 文件基础属性（原有）=====================
        QString file_path;       // TEXT NOT NULL UNIQUE
        qint64 file_size;        // INTEGER NOT NULL
        qint64 last_modified;    // INTEGER NOT NULL

        // ===================== 新增：音频基础属性（必加）=====================
        int length_in_seconds = 0; // 时长（秒），默认0
        int bitrate = 0;            // 码率（kbps），默认0
        int sample_rate = 0;        // 采样率（Hz），默认0
        int channels = 0;           // 声道数，默认0

        // ===================== 提取状态标识（原有）=====================
        bool metadata_extracted = false;
        bool cover_extracted = false;

        // ===================== 封面关联字段（原有）=====================
        QString cover_hash;      // CHAR(16) DEFAULT NULL

        // ===================== 核心元数据（原有）=====================
        QString title;
        QString artist;
        QString album;
        QString album_artist;
        QString genre;
        int year = -1;

        // ===================== 新增：核心元数据（推荐/必加）=====================
        QString composer;        // 作曲人，默认空
        QString lyrics;          // 歌词内容，默认空（TEXT长文本）

        // ===================== 新增：低频元数据（可选）=====================
        QString comment;         // 内嵌评论，默认空
        int bpm = 0;             // 节拍数，默认0

        // ===================== 数据库自动生成字段（原有）=====================
        qint64 created_at = 0;
        qint64 updated_at = 0;

        QString quality;         // 音质等级标签
        // 【默认构造函数】- Qt结构体必备，保证无参初始化
        MusicFileInfo() = default;

        // 【核心构造函数】- 快速初始化**必须非空**的核心字段（插入时的基础初始化）
        MusicFileInfo(const QString& hash, const QString& path, qint64 size, qint64 modTime)
            : file_hash(hash)
            , file_path(path)
            , file_size(size)
            , last_modified(modTime)
        {}

        // 【核心方法】将结构体转为QVariantMap，用于数据库insert操作
        // 关键：排除created_at/updated_at（数据库自动生成），NULL字段转为QVariant()（让SQLite存NULL）
        QVariantMap toVariantMap() const
        {
            QVariantMap map;
            // 原有字段：主键+文件基础属性
            map["file_hash"] = file_hash;
            map["file_path"] = file_path;
            map["file_size"] = file_size;
            map["last_modified"] = last_modified;

            // 新增：音频基础属性（数值型，直接传值，0表示未提取）
            map["length_in_seconds"] = length_in_seconds;
            map["bitrate"] = bitrate;
            map["sample_rate"] = sample_rate;
            map["channels"] = channels;

            // 原有字段：提取状态
            map["metadata_extracted"] = metadata_extracted;
            map["cover_extracted"] = cover_extracted;

            // 原有字段：封面哈希
            map["cover_hash"] = cover_hash.isEmpty() ? QVariant() : cover_hash;

            // 原有字段：核心元数据
            map["title"] = title;
            map["artist"] = artist;
            map["album"] = album.isEmpty() ? QVariant() : album;
            map["album_artist"] = album_artist.isEmpty() ? QVariant() : album_artist;
            map["genre"] = genre.isEmpty() ? QVariant() : genre;
            map["year"] = (year == -1) ? QVariant() : year;

            // 新增：核心元数据（作曲+歌词）
            map["composer"] = composer.isEmpty() ? QVariant() : composer;
            map["lyrics"] = lyrics.isEmpty() ? QVariant() : lyrics;

            // 新增：低频元数据（评论+BPM）
            map["comment"] = comment.isEmpty() ? QVariant() : comment;
            map["bpm"] = bpm;

            map["quality"] = quality.isEmpty() ? QVariant() : quality;

            // 排除数据库自动生成字段
            return map;
        }
        // 【校验方法】判断**非空约束字段**是否完整，避免插入无效数据
        // 返回true：核心字段均非空，可插入；返回false：核心字段缺失，不可插入
        bool isValid() const
        {
            return !file_hash.isEmpty()
                   && !file_path.isEmpty()
                   && file_size > 0
                   && last_modified > 0
                   && !title.isEmpty()
                   && !artist.isEmpty();
        }

        // 【便捷方法】从QFileInfo快速初始化文件基础属性（可选，简化扫描逻辑）
        void initFromFileInfo(const QFileInfo& fileInfo)
        {
            file_path = fileInfo.absoluteFilePath();
            file_size = fileInfo.size();
            last_modified = fileInfo.lastModified().toSecsSinceEpoch();
        }
    };

    auto& getMusicList() {return m_musicList;}
    const auto& getMusicList() const {return m_musicList;}
    void clearMusicList() { m_musicList.clear(); }
    QSet<QString> getAllMusicIds() const;
    QHash<QString, QVariantMap> getBatchMusicMetadata() const;
    MusicItem getMusicItemByFilePath(const QString& filePath) const;
    MusicItem getMusicItemByFileHash(const QString& fileHash) const;
    bool deleteMusicByHash(const QString& fileHash);
    bool restoreMusicByHash(const QString& fileHash);

    /** 查询所有 is_deleted=1 的记录（已删除音乐列表） */
    int getDeletedMusicCount() const;
    QVariantList getDeletedMusicList() const;
    /** 查询单条完整记录（含 updated_at / is_deleted），用于同步时间戳比较 */
    QVariantMap getMusicFileRecord(const QString& fileHash) const;
    /** 物理删除记录（文件已丢失时用），同时清理关联的 playlist_items */
    bool permanentlyRemoveMusic(const QString& fileHash);

    /** 查询 music_files 表中使用指定 coverHash 的歌曲数量 */
    int countByCoverHash(const QString& coverHash) const;

    int countCoverRefsAcrossTables(const QString& coverHash, const QString& excludeFileHash) const;

    static QString calculateMusicFileHash(const QFileInfo& fileInfo);
    static QString calculateMusicFileHash(const QString& absoluteFilePath);

    void startAsyncValidation();

    static QString classifyQuality(const QString& audioFormat, int bitrate, int sampleRate);
    static QString qualityColor(const QString& qualityLabel);

    // 去重：按标题+艺术家查询已有记录（用于扫描时自动去重）
    QList<QVariantMap> queryByTitleArtist(const QString& title, const QString& artist) const;

    // 恢复 is_deleted=1 的记录（insertIgnore 因主键冲突失败时用）
    bool restoreDeletedRecord(const MusicFileInfo& info);

    // 查询本地是否已有同 hash 的文件（用于下载前跳过）
    QString findExistingFileByHash(const QString& hash) const;

signals:
    void validationComplete(const QSet<QString>& missingHashes);

private:

    bool initializeTable();

    QList<MusicItem> m_musicList;

    IDatabaseService* m_databaseService = nullptr;

    const QString TABLE_NAME = "music_files";

};

#endif // MUSICFILEMANAGER_H

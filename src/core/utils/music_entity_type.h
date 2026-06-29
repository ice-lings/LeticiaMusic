#ifndef MUSICENTITYTYPE_H
#define MUSICENTITYTYPE_H

#include <QString>
#include <QSharedPointer>
#include <QDateTime>
#include <QImage>
#include <QUrl>

#include "fileref.h"


struct MusicItem {
    QString title = "";
    QString artist = "佚名";
    QString album = "";
    QString duration = "00:00";
    QUrl coverPath;
    QString coverHash;
    QString filePath;
    QString fileHash;
    QString sectionKey;
    QString pinyinFull;
    bool fileExists = true;
    QString quality = "";
};

struct MusicFileRef{
    QString filePath;
    QSharedPointer<TagLib::FileRef> fileRef;

    MusicFileRef() = default;
    MusicFileRef(const QString& path, const QSharedPointer<TagLib::FileRef>& ref)
        : filePath(path), fileRef(ref) {}
};

using MusicFileRefVector = QVector<MusicFileRef>;

struct MusicMetadata {
    int id = -1;
    QString title;
    QString artist;
    QString album;
    QString album_artist;
    QString genre;
    unsigned int year = 0;
    unsigned int track = 0;
    int track_total = 0;
    int disc_number = 0;
    int disc_total = 0;
    QString composer;
    QString lyrics;
    QString comment;
    int bpm = 0;
    QDateTime created_at;
    QDateTime updated_at;

    int lengthInSeconds = 0;
    int bitrate = 0;
    int sampleRate = 0;
    int channels = 0;
    QString audioFormat;

    QVariantMap toVariantMap() const;
};


inline static QString formatDuration(int seconds)
{
    if (seconds <= 0) {
        return "00:00"; // 无有效时长返回默认值
    }
    int min = seconds / 60;    // 分钟数
    int sec = seconds % 60;    // 剩余秒数
    // 补零格式化，确保分钟/秒都是两位
    return QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
}







#endif // MUSICENTITYTYPE_H

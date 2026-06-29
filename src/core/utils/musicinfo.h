#ifndef MUSICINFO_H
#define MUSICINFO_H

#include <QObject>
#include <QImage>
#include <QFileInfo>
#include <QCryptographicHash>

#include "music_entity_type.h"

class MusicInfo
{
public:
    MusicInfo();

public:
    void extraMusicInfo();
    QImage extraMusicCover(const TagLib::FileRef& fileRef);

    static QHash<QString, QString> generateMusicIds(const QVector<QString>& musicFilePaths)
    {
        QHash<QString, QString> newIdPathMap;
        newIdPathMap.reserve(musicFilePaths.size());

        for (const QString& path : musicFilePaths) {
            const QString id = generateId(QFileInfo(path));
            newIdPathMap[id] = path;
        }


        return newIdPathMap;
    }

    static QString generateId(const QFileInfo& fileInfo) {

        if (!fileInfo.exists() || !fileInfo.isFile()) {
            return "";
        }


        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            return "";
        }

        // 初始化哈希计算器：默认SHA1（40位，推荐作为主键），如需MD5（32位）可替换为QCryptographicHash::Md5
        QCryptographicHash hash(QCryptographicHash::Sha1);
        // 分段读取缓冲区：4096字节（适配大文件，避免内存溢出）
        const qint64 bufferSize = 4096;
        QByteArray buffer;

        while (!file.atEnd()) {
            buffer = file.read(bufferSize);
            if (buffer.isEmpty() && !file.atEnd()) {
                file.close();
                return "";
            }
            hash.addData(buffer);
        }


        file.close();

        // 计算最终哈希，转**纯小写十六进制字符串**返回（SHA1=40位，MD5=32位）
        QByteArray finalHash = hash.result();
        return finalHash.toHex().toLower();
    }
public:
    QString musicID;
    QString musicFilePath;
    MusicMetadata metadata;
    QString coverHash;
    QImage cover;

};

#endif // MUSICINFO_H



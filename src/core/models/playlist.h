#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QUrl>
#include <QObject>

struct Playlist {
    Q_GADGET
public:
    int id = -1;
    QString uuid;         // 跨设备唯一标识
    QString name;
    QString description;
    QString coverPath;
    bool isCollection = false;
    QString sourceFolder;
    QDateTime createdAt;
    QDateTime updatedAt;
    int musicCount = 0;
    
    Q_PROPERTY(int id MEMBER id)
    Q_PROPERTY(QString uuid MEMBER uuid)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString coverPath MEMBER coverPath)
    Q_PROPERTY(bool isCollection MEMBER isCollection)
    Q_PROPERTY(QString sourceFolder MEMBER sourceFolder)
    Q_PROPERTY(int musicCount MEMBER musicCount)
};

struct PlaylistMusicItem {
    Q_GADGET
public:
    QString musicHash;
    QString title;
    QString artist;
    QString album;
    QString duration;
    QString filePath;
    QString coverPath;
    int position = 0;

    Q_PROPERTY(QString musicHash MEMBER musicHash)
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString artist MEMBER artist)
    Q_PROPERTY(QString album MEMBER album)
    Q_PROPERTY(QString duration MEMBER duration)
    Q_PROPERTY(QString filePath MEMBER filePath)
    Q_PROPERTY(QString coverPath MEMBER coverPath)
    Q_PROPERTY(int position MEMBER position)
};

struct PlaylistItem {
    Q_GADGET
public:
    int id = -1;
    int playlistId = -1;
    QString musicHash;
    int position = 0;
    QString addedAt;
    QString musicTitle;
    QString musicArtist;
    QString musicFilePath;
    
    Q_PROPERTY(int id MEMBER id)
    Q_PROPERTY(int playlistId MEMBER playlistId)
    Q_PROPERTY(QString musicHash MEMBER musicHash)
    Q_PROPERTY(int position MEMBER position)
    Q_PROPERTY(QString addedAt MEMBER addedAt)
    Q_PROPERTY(QString musicTitle MEMBER musicTitle)
    Q_PROPERTY(QString musicArtist MEMBER musicArtist)
    Q_PROPERTY(QString musicFilePath MEMBER musicFilePath)
};

#endif // PLAYLIST_H

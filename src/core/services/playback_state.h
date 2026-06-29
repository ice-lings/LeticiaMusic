#ifndef PLAYBACK_STATE_H
#define PLAYBACK_STATE_H

#include "../utils/config_section.h"
#include "../application/play_source_type.h"

struct PlaybackState : public IConfigSection
{
    QString sectionKey() const override { return "playbackState"; }

    QString  lastPlayHash;
    QString  lastPlayFilePath;
    qint64   lastPlayPosition = 0;
    int      lastPlaylistId = -1;
    int      lastPlayIndex = 0;
    int      lastPlayMode = 0;
    PlaySourceType lastPlaySourceType = PlaySourceType::LocalMusic;

    QJsonObject serialize() const override
    {
        QJsonObject json;
        json["lastPlayHash"]       = lastPlayHash;
        json["lastPlayFilePath"]   = lastPlayFilePath;
        json["lastPlayPosition"]   = lastPlayPosition;
        json["lastPlaylistId"]     = lastPlaylistId;
        json["lastPlayIndex"]      = lastPlayIndex;
        json["lastPlayMode"]       = lastPlayMode;
        json["lastPlaySourceType"] = static_cast<int>(lastPlaySourceType);
        return json;
    }

    void deserialize(const QJsonObject& json) override
    {
        lastPlayHash       = json["lastPlayHash"].toString();
        lastPlayFilePath   = json["lastPlayFilePath"].toString();
        lastPlayPosition   = static_cast<qint64>(json["lastPlayPosition"].toDouble());
        lastPlaylistId     = json["lastPlaylistId"].toInt(-1);
        lastPlayIndex      = json["lastPlayIndex"].toInt(0);
        lastPlayMode       = json["lastPlayMode"].toInt(0);
        lastPlaySourceType = static_cast<PlaySourceType>(json["lastPlaySourceType"].toInt(0));
    }
};

#endif

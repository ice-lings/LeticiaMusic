#ifndef PLAY_SOURCE_TYPE_H
#define PLAY_SOURCE_TYPE_H

#include <QString>

enum class PlaySourceType {
    LocalMusic = 0,
    Playlist   = 1,
    Favorites  = 2
};

inline const char* playSourceTypeName(PlaySourceType t) {
    switch (t) {
    case PlaySourceType::LocalMusic: return "LocalMusic";
    case PlaySourceType::Playlist:   return "Playlist";
    case PlaySourceType::Favorites:  return "Favorites";
    }
    return "Unknown";
}

#endif // PLAY_SOURCE_TYPE_H
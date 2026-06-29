#include "playlistviewmodel.h"

PlaylistViewModel::PlaylistViewModel(PlaylistManager* manager, QObject* parent)
    : AbstractPlaylistListModel(manager, parent)
{
}

int PlaylistViewModel::createPlaylist(const QString& name, const QString& description)
{
    int id = createPlaylistImpl(name, description);
    if (id >= 0) {
        refresh();
        emit playlistCreated(id);
    }
    return id;
}

QVariantList PlaylistViewModel::getCollectionList()
{
    QVariantList result;
    auto collections = m_manager->getCollectionPlaylists();
    for (const auto& p : collections) {
        QVariantMap map;
        map["id"]   = p.id;
        map["name"] = p.name;
        map["musicCount"] = p.musicCount;
        result.append(map);
    }
    return result;
}

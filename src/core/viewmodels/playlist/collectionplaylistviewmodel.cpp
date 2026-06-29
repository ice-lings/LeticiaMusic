#include "collectionplaylistviewmodel.h"

CollectionPlaylistViewModel::CollectionPlaylistViewModel(PlaylistManager* manager, QObject* parent)
    : AbstractPlaylistListModel(manager, parent)
{
}

int CollectionPlaylistViewModel::createPlaylist(const QString& name)
{
    int id = createPlaylistImpl(name, "");
    if (id >= 0) {
        refresh();
        emit playlistCreated(id);
    }
    return id;
}

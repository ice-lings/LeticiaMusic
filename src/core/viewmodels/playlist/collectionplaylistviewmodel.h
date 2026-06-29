#ifndef COLLECTIONPLAYLISTVIEWMODEL_H
#define COLLECTIONPLAYLISTVIEWMODEL_H

#include "abstract_playlist_list_model.h"

class CollectionPlaylistViewModel : public AbstractPlaylistListModel
{
    Q_OBJECT
public:
    explicit CollectionPlaylistViewModel(PlaylistManager* manager, QObject* parent = nullptr);

    Q_INVOKABLE int createPlaylist(const QString& name);

protected:
    QVector<Playlist> fetchItems() override {
        return m_manager->getCollectionPlaylists();
    }
    int createPlaylistImpl(const QString& name, const QString& description) override {
        Q_UNUSED(description);
        return m_manager->createCollectionPlaylist(name, "");
    }
};

#endif // COLLECTIONPLAYLISTVIEWMODEL_H

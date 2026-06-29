#ifndef PLAYLISTVIEWMODEL_H
#define PLAYLISTVIEWMODEL_H

#include "abstract_playlist_list_model.h"

class PlaylistViewModel : public AbstractPlaylistListModel
{
    Q_OBJECT
public:
    explicit PlaylistViewModel(PlaylistManager* manager, QObject* parent = nullptr);

    Q_INVOKABLE int createPlaylist(const QString& name, const QString& description = "");
    Q_INVOKABLE QVariantList getCollectionList();

protected:
    QVector<Playlist> fetchItems() override {
        return m_manager->getAllPlaylists();
    }
    int createPlaylistImpl(const QString& name, const QString& description) override {
        return m_manager->createPlaylist(name, description);
    }
};

#endif // PLAYLISTVIEWMODEL_H

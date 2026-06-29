#ifndef ABSTRACTPLAYLISTLISTMODEL_H
#define ABSTRACTPLAYLISTLISTMODEL_H

#include "../../interfaces/IMusicEntityViewModel.h"
#include "../../models/playlist.h"
#include "../../playlist/playlistmanager.h"
#include <QVector>

class AbstractPlaylistListModel : public IMusicEntityViewModel
{
    Q_OBJECT
public:
    enum PlaylistRole {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        CoverPathRole,
        IsFavoriteRole,
        MusicCountRole,
        IsCollectionRole,
        SourceFolderRole
    };
    Q_ENUM(PlaylistRole)

    explicit AbstractPlaylistListModel(PlaylistManager* manager, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void initialize() override;
    void clearModel() override;
    void refreshModel() override;
    QObject* modelObject() override { return this; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool deletePlaylist(int id);
    Q_INVOKABLE bool renamePlaylist(int id, const QString& newName);
    Q_INVOKABLE Playlist getPlaylist(int id);

signals:
    void playlistCreated(int id);
    void playlistDeleted(int id);

protected:
    virtual QVector<Playlist> fetchItems() = 0;
    virtual int createPlaylistImpl(const QString& name, const QString& description) = 0;

    PlaylistManager* m_manager = nullptr;
    QVector<Playlist> m_items;
};

#endif

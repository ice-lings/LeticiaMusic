#include "abstract_playlist_list_model.h"

AbstractPlaylistListModel::AbstractPlaylistListModel(PlaylistManager* manager, QObject* parent)
    : IMusicEntityViewModel(parent), m_manager(manager)
{
}

int AbstractPlaylistListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_items.size();
}

QVariant AbstractPlaylistListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return QVariant();

    const Playlist& p = m_items.at(index.row());
    switch (role) {
    case IdRole:           return p.id;
    case NameRole:         return p.name;
    case DescriptionRole:  return p.description;
    case CoverPathRole:    return p.coverPath;
    case IsFavoriteRole:   return p.uuid == PlaylistManager::FAVORITE_PLAYLIST_UUID;
    case MusicCountRole:   return p.musicCount;
    case IsCollectionRole: return p.isCollection;
    case SourceFolderRole: return p.sourceFolder;
    default:               return QVariant();
    }
}

QHash<int, QByteArray> AbstractPlaylistListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole]           = "id";
    roles[NameRole]         = "name";
    roles[DescriptionRole]  = "description";
    roles[CoverPathRole]    = "coverPath";
    roles[IsFavoriteRole]   = "isFavorite";
    roles[MusicCountRole]   = "musicCount";
    roles[IsCollectionRole] = "isCollection";
    roles[SourceFolderRole] = "sourceFolder";
    return roles;
}

void AbstractPlaylistListModel::initialize()
{
    refresh();
    emit modelInitialized();
}

void AbstractPlaylistListModel::clearModel()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit modelUpdated();
}

void AbstractPlaylistListModel::refreshModel()
{
    refresh();
    emit modelUpdated();
}

void AbstractPlaylistListModel::refresh()
{
    beginResetModel();
    m_items = fetchItems();
    endResetModel();
}

bool AbstractPlaylistListModel::deletePlaylist(int id)
{
    bool ok = m_manager->deletePlaylist(id);
    if (ok) {
        refresh();
        emit playlistDeleted(id);
    }
    return ok;
}

bool AbstractPlaylistListModel::renamePlaylist(int id, const QString& newName)
{
    bool ok = m_manager->renamePlaylist(id, newName);
    if (ok) {
        refresh();
    }
    return ok;
}

Playlist AbstractPlaylistListModel::getPlaylist(int id)
{
    return m_manager->getPlaylist(id);
}

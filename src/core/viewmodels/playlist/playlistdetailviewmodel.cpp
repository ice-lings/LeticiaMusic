#include "playlistdetailviewmodel.h"
#include "../../utils/logger.h"

PlaylistDetailViewModel::PlaylistDetailViewModel(PlaylistManager* manager, QObject *parent)
    : IMusicEntityViewModel(parent), m_manager(manager)
{
    if (m_manager) {
        connect(m_manager, &PlaylistManager::musicAddedToPlaylist,
                this, &PlaylistDetailViewModel::onMusicAddedToPlaylist);
        connect(m_manager, &PlaylistManager::musicRemovedFromPlaylist,
                this, &PlaylistDetailViewModel::onMusicRemovedFromPlaylist);
    }
}

void PlaylistDetailViewModel::onMusicAddedToPlaylist(int playlistId, const QString &musicHash)
{
    Q_UNUSED(musicHash);
    if (playlistId == m_playlistId) {
        refresh();
    }
}

void PlaylistDetailViewModel::onMusicRemovedFromPlaylist(int playlistId, const QString &musicHash)
{
    Q_UNUSED(musicHash);
    if (playlistId == m_playlistId) {
        refresh();
    }
}

int PlaylistDetailViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_musicItems.size();
}

QVariant PlaylistDetailViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_musicItems.size()) {
        return QVariant();
    }

    const PlaylistMusicItem &item = m_musicItems[index.row()];

    switch (role) {
    case MusicHashRole:
        return item.musicHash;
    case TitleRole:
        return item.title;
    case ArtistRole:
        return item.artist;
    case AlbumRole:
        return item.album;
    case DurationRole:
        return item.duration;
    case FilePathRole:
        return item.filePath;
    case CoverPathRole:
        return item.coverPath;
    case PositionRole:
        return item.position;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PlaylistDetailViewModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[MusicHashRole] = "musicHash";
    roles[TitleRole] = "title";
    roles[ArtistRole] = "artist";
    roles[AlbumRole] = "album";
    roles[DurationRole] = "duration";
    roles[FilePathRole] = "filePath";
    roles[CoverPathRole] = "coverPath";
    roles[PositionRole] = "position";
    return roles;
}

void PlaylistDetailViewModel::setPlaylistId(int id)
{
    if (m_playlistId != id) {
        m_playlistId = id;
        loadPlaylist(id);
        emit playlistIdChanged();
    }
}

QString PlaylistDetailViewModel::playlistName() const
{
    return m_playlistName;
}

void PlaylistDetailViewModel::refresh()
{
    if (m_playlistId >= 0) {
        loadPlaylist(m_playlistId);
    }
}

void PlaylistDetailViewModel::loadPlaylist(int playlistId)
{
    beginResetModel();
    m_musicItems = m_manager->getPlaylistMusicItems(playlistId);
    
    m_playlistId = playlistId;
    emit playlistIdChanged();
    
    Playlist playlist = m_manager->getPlaylist(playlistId);
    if (m_playlistName != playlist.name) {
        m_playlistName = playlist.name;
    }
    m_isCollection = playlist.isCollection;
    
    endResetModel();
    emit countChanged();
    
    Log.debug(QString("Loaded playlist %1 with %2 items (isCollection=%3)").arg(playlistId).arg(m_musicItems.size()).arg(m_isCollection));
}

void PlaylistDetailViewModel::clear()
{
    beginResetModel();
    m_musicItems.clear();
    m_playlistId = -1;
    m_playlistName.clear();
    endResetModel();
    emit countChanged();
}

QVariantList PlaylistDetailViewModel::getAllItems() const
{
    QVariantList result;
    for (const PlaylistMusicItem &item : m_musicItems) {
        QVariantMap map;
        map.insert("musicHash", item.musicHash);
        map.insert("title", item.title);
        map.insert("artist", item.artist);
        map.insert("album", item.album);
        map.insert("duration", item.duration);
        map.insert("filePath", item.filePath);
        map.insert("coverPath", item.coverPath);
        result.append(map);
    }
    return result;
}

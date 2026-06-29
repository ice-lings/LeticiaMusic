#ifndef PLAYLISTDETAILVIEWMODEL_H
#define PLAYLISTDETAILVIEWMODEL_H

#include <QVector>
#include "../../interfaces/IMusicEntityViewModel.h"
#include "../../models/playlist.h"
#include "../../playlist/playlistmanager.h"

class PlaylistDetailViewModel : public IMusicEntityViewModel
{
    Q_OBJECT

    Q_PROPERTY(int playlistId READ playlistId WRITE setPlaylistId NOTIFY playlistIdChanged)
    Q_PROPERTY(QString playlistName READ playlistName NOTIFY playlistIdChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool isCollection READ isCollection NOTIFY playlistIdChanged)

public:
    enum MusicRole {
        MusicHashRole = Qt::UserRole + 1,
        TitleRole,
        ArtistRole,
        AlbumRole,
        DurationRole,
        FilePathRole,
        CoverPathRole,
        PositionRole
    };
    Q_ENUM(MusicRole)

    explicit PlaylistDetailViewModel(PlaylistManager* manager, QObject *parent = nullptr);

    void initialize() override {
        emit modelInitialized();
    }
    void clearModel() override {
        clear();
        emit modelUpdated();
    }
    void refreshModel() override {
        refresh();
        emit modelUpdated();
    }
    QObject* modelObject() override { return this; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int playlistId() const { return m_playlistId; }
    void setPlaylistId(int id);
    QString playlistName() const;
    bool isCollection() const { return m_isCollection; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void loadPlaylist(int playlistId);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariantList getAllItems() const;

signals:
    void playlistIdChanged();
    void playlistNameChanged();
    void countChanged();

private slots:
    void onMusicAddedToPlaylist(int playlistId, const QString &musicHash);
    void onMusicRemovedFromPlaylist(int playlistId, const QString &musicHash);

private:
    PlaylistManager* m_manager = nullptr;
    int m_playlistId = -1;
    QString m_playlistName;
    bool m_isCollection = false;
    QVector<PlaylistMusicItem> m_musicItems;
};

#endif // PLAYLISTDETAILVIEWMODEL_H

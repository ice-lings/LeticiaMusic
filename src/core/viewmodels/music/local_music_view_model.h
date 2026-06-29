#ifndef LocalMusicViewModel_H
#define LocalMusicViewModel_H

#include <QAbstractListModel>
#include <QVector>
#include <QUrl>
#include <QSet>

#include "../../interfaces/IMusicEntityViewModel.h"
#include "../../utils/music_entity_type.h"

class LocalMusicViewModel : public IMusicEntityViewModel
{
   Q_OBJECT
   Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum MusicItemRole {
        TitleRole = Qt::UserRole + 1,
        ArtistRole,
        AlbumRole,
        DurationRole,
        FilePathRole,
        CoverArtRole,
        MusicHashRole,
        SectionKeyRole,
        FileExistsRole,
        QualityRole,
        QualityColorRole
    };
   Q_ENUM(MusicItemRole)

    explicit LocalMusicViewModel(QObject *parent = nullptr);

    // IMusicEntityViewModel Begin
    void initialize() override;

    void clearModel() override;

    void refreshModel() override;

    QObject* modelObject() override;
    // IMusicEntityViewModel End

    // QAbstractListModel Begin
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;
    // QAbstractListModel End


    // QML可调用方法
    Q_INVOKABLE void loadFromDirectory(const QString &directory);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QUrl getFilePathAt(int index) const;
    Q_INVOKABLE QVariantList getAllItems() const;

    Q_INVOKABLE QVariantMap get(int index) const;

    /** 当前列表中文件路径所在行，找不到返回 -1 */
    Q_INVOKABLE int indexForFilePath(const QString &filePath) const;
    /** 行内音频文件的本地路径，无效索引返回空字符串 */
    QString audioFilePathAt(int row) const;

    /** 按 fileHash 移除歌曲，成功返回 true */
    Q_INVOKABLE bool removeByHash(const QString &musicHash);

    Q_INVOKABLE bool updateMetadataByHash(const QString &musicHash, const QString &title,
                                           const QString &artist, const QString &album);

    Q_INVOKABLE bool updateCoverByHash(const QString &musicHash, const QString &coverHash,
                                        const QUrl &coverPath);

    Q_INVOKABLE QVariantList search(const QString& keyword, int maxResults = 20) const;

    void refreshFromDB(const QList<MusicItem>& items);
    void appendItem(const MusicItem& item);

signals:
    void countChanged();
    void loadingStarted();
    void loadingFinished(int count);
    void errorOccurred(const QString &message);

public slots:
    void onMetaDataReady(const QList<MusicItem> &metadataList);

    void onValidationComplete(const QSet<QString>& missingHashes);

    void sortPlaylist();

private:
    QVector<MusicItem> m_playlist;
};

#endif // LocalMusicViewModel_H

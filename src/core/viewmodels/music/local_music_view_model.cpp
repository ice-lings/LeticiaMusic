#include "local_music_view_model.h"

#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <algorithm>

#include "../../utils/logger.h"
#include "../../utils/musicinfo.h"
#include "../../utils/pinyin.h"
#include "../../filesystem/musicfile/musicfilemanager.h"

LocalMusicViewModel::LocalMusicViewModel(QObject *parent)
    : IMusicEntityViewModel(parent)
{
}

void LocalMusicViewModel::initialize()
{
    Log.info("LocalMusicViewModel initialized: " + viewModelName());
    emit modelInitialized();
}

void LocalMusicViewModel::clearModel()
{
    if (!m_playlist.isEmpty()) {
        beginResetModel();
        m_playlist.clear();
        endResetModel();
        emit countChanged();
        emit modelUpdated();
    }
}

void LocalMusicViewModel::refreshModel()
{
    Log.info("Refreshing local music model: " + viewModelName());
    // 重新发出数据更新信号，触发视图刷新
    emit dataChanged(index(0), index(m_playlist.size() - 1));
    emit modelUpdated();
}

QObject *LocalMusicViewModel::modelObject()
{
    return this;
}

int LocalMusicViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_playlist.size();
}

QVariant LocalMusicViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_playlist.size()) {
        return QVariant();
    }

    const MusicItem &item = m_playlist.at(index.row());

    switch (role) {
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
    case CoverArtRole:
        return item.coverPath;
    case MusicHashRole:
        return item.fileHash;
    case SectionKeyRole:
        return item.sectionKey;
    case FileExistsRole:
        return item.fileExists;
    case QualityRole:
        return item.quality;
    case QualityColorRole: {
        // 颜色映射（与MusicFileManager::qualityColor保持一致）
        if (item.quality == "HI") return "#E5C07B";
        if (item.quality == "SQ") return "#C678DD";
        if (item.quality == "HQ") return "#61AFEF";
        if (item.quality == "SD") return "#98C379";
        return "transparent"; // LQ 或空
    }
    default:
        return QVariant();
    }
}


QHash<int, QByteArray> LocalMusicViewModel::roleNames() const
{
    static QHash<int, QByteArray> roles {
        {TitleRole, "title"},
        {ArtistRole, "artist"},
        {AlbumRole, "album"},
        {DurationRole, "duration"},
        {FilePathRole, "filePath"},
        {CoverArtRole, "coverArt"},
        {MusicHashRole, "musicHash"},
        {SectionKeyRole, "sectionKey"},
        {FileExistsRole, "fileExists"},
        {QualityRole, "quality"},
        {QualityColorRole, "qualityColor"}
    };
    return roles;
}

void LocalMusicViewModel::loadFromDirectory(const QString &directory)
{
    emit loadingStarted();

    QDir dir(directory);
    if (!dir.exists()) {
        emit errorOccurred("Directory not found: " + directory);
        return;
    }

    QStringList filters;
    filters << "*.mp3" << "*.flac" << "*.wav" << "*.ogg";
    auto files = dir.entryInfoList(filters, QDir::Files);

    beginResetModel();
    m_playlist.clear();

    for (const QFileInfo &fileInfo : files) {
        MusicItem item;
        item.filePath = fileInfo.absoluteFilePath();
        item.fileHash = MusicFileManager::calculateMusicFileHash(fileInfo.absoluteFilePath());

        MusicInfo musicInfo;
        musicInfo.musicFilePath = fileInfo.absoluteFilePath();
        musicInfo.extraMusicInfo();

        item.title = musicInfo.metadata.title.isEmpty() ? fileInfo.completeBaseName() : musicInfo.metadata.title;
        item.artist = musicInfo.metadata.artist.isEmpty() ? "未知艺术家" : musicInfo.metadata.artist;
        item.album = musicInfo.metadata.album;
        item.duration = formatDuration(musicInfo.metadata.lengthInSeconds);

        if (!musicInfo.cover.isNull()) {
            QString coverPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + "_cover.jpg";
            if (musicInfo.cover.save(coverPath, "JPG")) {
                item.coverPath = QUrl::fromLocalFile(coverPath);
            }
        }

        m_playlist.append(item);
    }

    endResetModel();
    emit countChanged();
    emit loadingFinished(m_playlist.size());
}

void LocalMusicViewModel::clear()
{
    beginResetModel();
    m_playlist.clear();
    endResetModel();
    emit countChanged();
}

QUrl LocalMusicViewModel::getFilePathAt(int index) const
{
    if (index >= 0 && index < m_playlist.size())
        return QUrl::fromLocalFile(m_playlist.at(index).filePath);
    return QUrl();
}

int LocalMusicViewModel::indexForFilePath(const QString &filePath) const
{
    if (filePath.isEmpty())
        return -1;
    const QString absIn = QFileInfo(filePath).absoluteFilePath();
    for (int i = 0; i < m_playlist.size(); ++i) {
        if (QFileInfo(m_playlist.at(i).filePath).absoluteFilePath() == absIn)
            return i;
    }
    return -1;
}

QString LocalMusicViewModel::audioFilePathAt(int row) const
{
    if (row >= 0 && row < m_playlist.size())
        return m_playlist.at(row).filePath;
    return {};
}

QVariantList LocalMusicViewModel::getAllItems() const
{
    QVariantList result;
    for (const MusicItem &item : m_playlist) {
        QVariantMap map;
        map.insert("musicHash", item.fileHash);
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

QVariantMap LocalMusicViewModel::get(int index) const
{
    if (index < 0 || index >= m_playlist.size())
        return QVariantMap();
    const MusicItem &item = m_playlist.at(index);
    QVariantMap map;
    map.insert("title", item.title);
    map.insert("artist", item.artist);
    map.insert("album", item.album);
    map.insert("duration", item.duration);
    map.insert("filePath", item.filePath);
    map.insert("coverArt", item.coverPath);
    map.insert("musicHash", item.fileHash);
    map.insert("sectionKey", item.sectionKey);
    return map;
}

void LocalMusicViewModel::onMetaDataReady(const QList<MusicItem> &metadataList)
{
    if (metadataList.isEmpty()) {
        Log.warning("[LocalMusicViewModel] Received empty metadata list");
        return;
    }

    beginResetModel();
    m_playlist.append(metadataList.begin(), metadataList.end());
    sortPlaylist();
    endResetModel();
    emit countChanged();
}

void LocalMusicViewModel::onValidationComplete(const QSet<QString>& missingHashes)
{
    if (missingHashes.isEmpty() || m_playlist.isEmpty()) return;

    int firstChanged = -1;
    int lastChanged = -1;

    for (int i = 0; i < m_playlist.size(); ++i) {
        if (missingHashes.contains(m_playlist[i].fileHash)) {
            m_playlist[i].fileExists = false;
            if (firstChanged == -1) firstChanged = i;
            lastChanged = i;
        }
    }

    if (firstChanged != -1) {
        emit dataChanged(index(firstChanged), index(lastChanged),
                         {FileExistsRole});
    }
}

void LocalMusicViewModel::sortPlaylist()
{
    if (m_playlist.isEmpty()) return;

    Log.info(QString("[Sort] begin: count=%1").arg(m_playlist.size()));
    QElapsedTimer sortTimer;
    sortTimer.start();

    // 分组优先级: A-Z = 0..25, '0'(数字) = 26, '#'(其他) = 27
    auto sectionPriority = [](const QString &key) -> int {
        if (key.isEmpty()) return 27;
        QChar c = key.at(0);
        if (c >= 'A' && c <= 'Z') return c.unicode() - 'A';
        if (c == '0') return 26;
        return 27;
    };

    for (auto &item : m_playlist) {
        if (item.sectionKey.isEmpty()) {
            item.sectionKey = PinyinUtils::sectionKey(item.title);
        }
    }

    std::stable_sort(m_playlist.begin(), m_playlist.end(),
        [&sectionPriority](const MusicItem &a, const MusicItem &b) {
            int ga = sectionPriority(a.sectionKey);
            int gb = sectionPriority(b.sectionKey);
            if (ga != gb) return ga < gb;

            QString pa = a.pinyinFull.isEmpty()
                ? PinyinUtils::toPinyin(a.title).toLower() : a.pinyinFull;
            QString pb = b.pinyinFull.isEmpty()
                ? PinyinUtils::toPinyin(b.title).toLower() : b.pinyinFull;
            if (pa != pb) return pa < pb;
            return a.title.toLower() < b.title.toLower();
        });

    Log.info(QString("[Sort] done: took %1ms").arg(sortTimer.elapsed()));
}

bool LocalMusicViewModel::removeByHash(const QString &musicHash)
{
    if (musicHash.isEmpty()) return false;

    for (int i = 0; i < m_playlist.size(); ++i) {
        if (m_playlist.at(i).fileHash == musicHash) {
            beginRemoveRows(QModelIndex(), i, i);
            m_playlist.removeAt(i);
            endRemoveRows();
            emit countChanged();
            return true;
        }
    }
    return false;
}

bool LocalMusicViewModel::updateMetadataByHash(const QString &musicHash, const QString &title,
                                                 const QString &artist, const QString &album)
{
    if (musicHash.isEmpty()) return false;

    for (int i = 0; i < m_playlist.size(); ++i) {
        if (m_playlist.at(i).fileHash == musicHash) {
            if (!title.isEmpty())  m_playlist[i].title = title;
            if (!artist.isEmpty()) m_playlist[i].artist = artist;
            if (!album.isEmpty())  m_playlist[i].album = album;
            QModelIndex idx = index(i, 0);
            emit dataChanged(idx, idx, {TitleRole, ArtistRole, AlbumRole});
            return true;
        }
    }
    return false;
}

bool LocalMusicViewModel::updateCoverByHash(const QString &musicHash, const QString &coverHash,
                                              const QUrl &coverPath)
{
    if (musicHash.isEmpty()) return false;

    for (int i = 0; i < m_playlist.size(); ++i) {
        if (m_playlist.at(i).fileHash == musicHash) {
            m_playlist[i].coverHash = coverHash;
            m_playlist[i].coverPath = coverPath;
            QModelIndex idx = index(i, 0);
            emit dataChanged(idx, idx, {CoverArtRole});
            return true;
        }
    }
    return false;
}

static QString highlightKeyword(const QString& text, const QString& keyword)
{
    if (keyword.isEmpty() || text.isEmpty()) return text.toHtmlEscaped();
    QString result = text.toHtmlEscaped();
    QString kw = keyword.toLower();

    int pos = 0;
    while (pos < result.length()) {
        pos = result.toLower().indexOf(kw, pos);
        if (pos < 0) break;
        result = result.left(pos)
                 + "<font color=\"#FFD54F\"><b>"
                 + result.mid(pos, keyword.length())
                 + "</b></font>"
                 + result.mid(pos + keyword.length());
        pos += 36 + keyword.length();
    }
    return result;
}

QVariantList LocalMusicViewModel::search(const QString& keyword, int maxResults) const
{
    QVariantList results;
    if (keyword.trimmed().isEmpty() || maxResults <= 0) return results;

    QString kw = keyword.trimmed().toLower();
    int found = 0;

    for (int i = 0; i < m_playlist.size(); ++i) {
        const MusicItem& item = m_playlist.at(i);
        if (item.title.toLower().contains(kw) ||
            item.artist.toLower().contains(kw) ||
            item.album.toLower().contains(kw)) {

            QVariantMap map;
            map.insert("title", item.title);
            map.insert("artist", item.artist);
            map.insert("titleHighlighted", highlightKeyword(item.title, keyword.trimmed()));
            map.insert("artistHighlighted", highlightKeyword(item.artist, keyword.trimmed()));
            map.insert("coverPath", item.coverPath);
            map.insert("musicHash", item.fileHash);
            map.insert("modelIndex", i);
            results.append(map);

            if (++found >= maxResults) break;
        }
    }
    return results;
}

void LocalMusicViewModel::refreshFromDB(const QList<MusicItem>& items)
{
    beginResetModel();
    m_playlist.clear();
    for (const auto& item : items) {
        m_playlist.append(item);
    }
    sortPlaylist();
    endResetModel();
    emit countChanged();
}

void LocalMusicViewModel::appendItem(const MusicItem& item)
{
    m_playlist.append(item);
    sortPlaylist();
    beginResetModel();
    endResetModel();
    emit countChanged();
}








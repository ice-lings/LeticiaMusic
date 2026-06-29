#include "scan_service.h"
#include "../application/appconfig.h"
#include "../application/scanworker.h"
#include "../filesystem/musicfile/musicfilemanager.h"
#include "../filesystem/musicfile/musicscanner.h"
#include "../filesystem/metadata/musiccovermanager.h"
#include "../filesystem/metadata/metadataoverridemanager.h"
#include "../viewmodels/viewmodelmanager.h"
#include "../viewmodels/music/local_music_view_model.h"
#include "../viewmodels/playlist/playlistviewmodel.h"
#include "../playlist/playlistmanager.h"
#include "../utils/pinyin.h"
#include "../utils/logger.h"
#include "../utils/platform_path_helper.h"
#include "../utils/musicinfo.h"
#include <QThread>
#include <QTimer>
#include <QFileInfo>
#include <QUrl>
#include <QStandardPaths>
#include <QDirIterator>
#ifdef Q_OS_ANDROID
#include <android/log.h>
#include <QtCore/private/qandroidextras_p.h>
#include "../platform/android_media_scanner.h"
#endif

ScanService::ScanService(IDatabaseService* db, MusicFileManager* mfm,
                         MusicScanner* scanner, MusicCoverManager* coverMgr,
                         MetadataOverrideManager* overrideMgr,
                         ViewModelManager* vmMgr, QObject* parent)
    : QObject(parent)
    , m_db(db)
    , m_fileManager(mfm)
    , m_scanner(scanner)
    , m_coverManager(coverMgr)
    , m_overrideManager(overrideMgr)
    , m_vmManager(vmMgr)
{
}

void ScanService::scanMusicNow()
{
#ifdef Q_OS_ANDROID
    auto future = QtAndroidPrivate::checkPermission("android.permission.READ_MEDIA_AUDIO");
    future.waitForFinished();
    if (future.result() == QtAndroidPrivate::PermissionResult::Authorized) {
        Log.info("[权限] READ_MEDIA_AUDIO 已授权，开始扫描");
        doScanMusicNow();
    } else {
        Log.warning("[权限] READ_MEDIA_AUDIO 未授权，请求权限...");
        Log.warning("[权限] 请在弹出对话框中授予「音乐和音频」权限");
        QtAndroidPrivate::requestPermission("android.permission.READ_MEDIA_AUDIO");
        QTimer::singleShot(3000, this, [this]() {
            auto f2 = QtAndroidPrivate::checkPermission("android.permission.READ_MEDIA_AUDIO");
            f2.waitForFinished();
            if (f2.result() == QtAndroidPrivate::PermissionResult::Authorized) {
                Log.info("[权限] READ_MEDIA_AUDIO 已授权，开始扫描");
                doScanMusicNow();
            } else {
                Log.warning("[权限] 用户未授予 READ_MEDIA_AUDIO，扫描可能返回空列表");
                doScanMusicNow();
            }
        });
    }
#else
    doScanMusicNow();
#endif
}

void ScanService::doScanMusicNow()
{
    if (m_scanning) {
        Log.info("Scan already in progress, ignoring");
        return;
    }

    auto dirs = AppConfig::instance().musicScanDirectories();
    if (dirs.isEmpty()) {
        dirs << QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        AppConfig::instance().setMusicScanDirectories(dirs);
    }

    const QSet<QString> existingIds = m_fileManager->getAllMusicIds();

    m_scanThread = new QThread(this);
#ifdef Q_OS_ANDROID
    auto androidDirs = AppConfig::instance().musicScanDirectories();
    if (androidDirs.isEmpty()) {
        androidDirs << QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        AppConfig::instance().setMusicScanDirectories(androidDirs);
    }

    QStringList mediaFiles = AndroidMediaScanner::queryAllAudio();

    const QString downloadDir = PlatformPathHelper::downloadDir();
    static const QSet<QString> audioExts = {"mp3","flac","wav","ogg","m4a","aac","wma","ape"};
    QDirIterator dit(downloadDir, QDir::Files, QDirIterator::Subdirectories);
    while (dit.hasNext()) {
        QString path = dit.next();
        QString ext = QFileInfo(path).suffix().toLower();
        if (audioExts.contains(ext)) {
            mediaFiles.append(path);
        }
    }
    Log.info(QString("[Android] 扫描发现 %1 个音频文件 (MediaStore + 下载目录)").arg(mediaFiles.size()));
    m_scanWorker = new ScanWorker(mediaFiles,
                                    m_scanner,
                                    m_coverManager,
                                    existingIds,
                                    true,
                                    AppConfig::instance().minFileSizeKB(),
                                    AppConfig::instance().minDurationSec());
#else
    Log.info(QString("[Scan] Windows 扫描目录: %1 (共 %2 个)")
        .arg(AppConfig::instance().musicScanDirectories().join(", "))
        .arg(AppConfig::instance().musicScanDirectories().size()));
    m_scanWorker = new ScanWorker(AppConfig::instance().musicScanDirectories(),
                                    m_scanner,
                                    m_coverManager,
                                    existingIds,
                                    AppConfig::instance().minFileSizeKB(),
                                    AppConfig::instance().minDurationSec());
#endif
    m_scanWorker->moveToThread(m_scanThread);

    connect(m_scanWorker, &ScanWorker::phaseChanged, this, [this](int phase) {
        m_scanPhase = phase;
        emit scanPhaseChanged();
    }, Qt::QueuedConnection);

    connect(m_scanWorker, &ScanWorker::phaseTextChanged, this, [this](const QString& text) {
        m_scan_phaseText = text;
        emit scanPhaseChanged();
    }, Qt::QueuedConnection);

    connect(m_scanWorker, &ScanWorker::progressChanged, this, [this](int current, int total, const QString& file) {
        m_scan_current = current;
        m_scan_total = total;
        m_scan_currentFile = file;
        emit scanProgressChanged();
    }, Qt::QueuedConnection);

    connect(m_scanWorker, &ScanWorker::scanFinished, this, [this](int totalAdded, const QList<ScanResultItem>& items) {
        if (m_collectionImportActive) {
            finishCollectionImport(totalAdded, items);
        } else {
            finishScanFromWorker(totalAdded, items);
        }
    }, Qt::QueuedConnection);

    connect(m_scanWorker, &ScanWorker::errorOccurred, this, [this](const QString& err) {
        Log.error("Scan error: " + err);
        finishScanFromWorker(0, {});
    }, Qt::QueuedConnection);

    connect(m_scanThread, &QThread::finished, m_scanWorker, &QObject::deleteLater);
    connect(m_scanThread, &QThread::finished, m_scanThread, &QObject::deleteLater);

    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(200);
    connect(m_progressTimer, &QTimer::timeout, this, [this]() {
        if (m_scanWorker) {
            const int progress = m_scanWorker->progressValue();
            if (progress != m_scan_current) {
                m_scan_current = progress;
                emit scanProgressChanged();
            }
        }
    }, Qt::QueuedConnection);
    m_progressTimer->start();

    m_scanning = true;
    emit scanningChanged();

    m_scanThread->start();
    QMetaObject::invokeMethod(m_scanWorker, "process", Qt::QueuedConnection);
}

void ScanService::finishScanFromWorker(int totalAdded, const QList<ScanResultItem>& items)
{
    if (!m_scanning) return;

    if (m_progressTimer) {
        m_progressTimer->stop();
        delete m_progressTimer;
        m_progressTimer = nullptr;
    }

    if (!items.isEmpty()) {
        auto& musicList = m_fileManager->getMusicList();
        QList<MusicItem> vmItems;

        int fromDownloadDir = 0;
        int excludedByScanFolder = 0;
        const QString dlDir = PlatformPathHelper::downloadDir();

        QSet<QString> uniqueCoverHashes;
        for (const auto& item : items) {
            if (!item.coverHash.isEmpty()) {
                uniqueCoverHashes.insert(item.coverHash);
            }
        }

        int batchSize = (items.size() <= 50) ? items.size() :
                        (items.size() <= 500) ? 100 : 200;

        m_db->beginTransaction();

        for (const QString& hash : uniqueCoverHashes) {
            m_coverManager->persistCoverToDb(hash);
        }

        for (int i = 0; i < items.size(); ++i) {
            const auto& item = items[i];

#ifdef Q_OS_ANDROID
            bool excluded = false;
            for (const auto& sf : AppConfig::instance().scanFolders) {
                if (!sf.enabled && item.filePath.startsWith(sf.path + "/")) {
                    excluded = true;
                    break;
                }
            }
            if (excluded) {
                excludedByScanFolder++;
                totalAdded--;
                continue;
            }
#endif

            if (item.filePath.startsWith(dlDir)) fromDownloadDir++;

            if (batchSize > 0 && i > 0 && i % batchSize == 0) {
                m_db->commitTransaction();
                m_db->beginTransaction();
            }

            MusicItem mi;
            mi.fileHash = item.fileHash;
            mi.filePath = item.filePath;
            mi.title = item.title;
            mi.artist = item.artist;
            mi.album = item.album;
            mi.duration = item.duration;
            mi.coverHash = item.coverHash;
            mi.sectionKey = PinyinUtils::sectionKey(item.title.isEmpty() ? QFileInfo(item.filePath).completeBaseName() : item.title);
            if (!item.coverHash.isEmpty()) {
                mi.coverPath = QUrl::fromLocalFile(m_coverManager->getCoverPath(item.coverHash));
            }

            MusicInfo info;
            info.musicID = item.fileHash;
            info.musicFilePath = item.filePath;
            info.metadata = item.metadata;
            info.coverHash = item.coverHash;
            info.cover = item.cover;

            mi.quality = MusicFileManager::classifyQuality(
                item.metadata.audioFormat, item.metadata.bitrate, item.metadata.sampleRate);

            m_fileManager->trySaveMusicFileInfoToDB(info, mi);

            musicList.append(mi);
            vmItems.append(mi);
        }

        Log.info(QString("[Scan] 入库统计: 总数=%1 来源下载目录=%2 排除目录=%3 提交=%4")
            .arg(items.size()).arg(fromDownloadDir).arg(excludedByScanFolder)
            .arg(vmItems.size()));

        m_db->commitTransaction();

        applyMetadataOverrides(vmItems);

        if (auto* localViewModel = m_vmManager->getViewModel<LocalMusicViewModel>("local_music")) {
            localViewModel->onMetaDataReady(vmItems);
        }
    }

#ifdef Q_OS_ANDROID
    rebuildScanFoldersFromLibrary();
#endif

    m_scanning = false;
    m_scanPhase = 0;
    m_scan_phaseText = "";
    m_scan_currentFile = "";
    m_scan_current = 0;
    m_scan_total = 0;
    emit scanningChanged();
    emit scanPhaseChanged();
    emit scanProgressChanged();
    emit scanCompleted(totalAdded);

    if (m_scanThread) {
        m_scanThread->quit();
    }

    m_scanWorker = nullptr;
    m_scanThread = nullptr;

    Log.info(QString("扫描完成，新增 %1 个文件").arg(totalAdded));
}

void ScanService::importCollectionFolder(const QString& folderPath, const QString& name)
{
    if (m_scanning) {
        Log.warning("Scan already in progress, cannot import collection now");
        return;
    }

    QDir dir(folderPath);
    if (!dir.exists()) {
        Log.error("Import collection: folder does not exist: " + folderPath);
        return;
    }

    m_collectionImportActive = true;
    m_pendingCollectionName = name;
    m_pendingCollectionPath = folderPath;

    const QSet<QString> existingIds = m_fileManager->getAllMusicIds();

    m_scanThread = new QThread(this);
    m_scanWorker = new ScanWorker(QStringList{folderPath},
                                    m_scanner,
                                    m_coverManager,
                                    existingIds);
    m_scanWorker->moveToThread(m_scanThread);

    connect(m_scanWorker, &ScanWorker::phaseChanged, this, [this](int phase) {
        m_scanPhase = phase;
        emit scanPhaseChanged();
    }, Qt::QueuedConnection);

    connect(m_scanWorker, &ScanWorker::phaseTextChanged, this, [this](const QString& text) {
        m_scan_phaseText = text;
        emit scanPhaseChanged();
    }, Qt::QueuedConnection);

    connect(m_scanWorker, &ScanWorker::progressChanged, this, [this](int current, int total, const QString& file) {
        m_scan_current = current;
        m_scan_total = total;
        m_scan_currentFile = file;
        emit scanProgressChanged();
    }, Qt::QueuedConnection);

    connect(m_scanWorker, &ScanWorker::scanFinished, this, [this](int totalAdded, const QList<ScanResultItem>& items) {
        if (m_collectionImportActive) {
            finishCollectionImport(totalAdded, items);
        } else {
            finishScanFromWorker(totalAdded, items);
        }
    }, Qt::QueuedConnection);

    connect(m_scanWorker, &ScanWorker::errorOccurred, this, [this](const QString& err) {
        Log.error("Collection import scan error: " + err);
        m_collectionImportActive = false;
        finishScanFromWorker(0, {});
    }, Qt::QueuedConnection);

    connect(m_scanThread, &QThread::finished, m_scanWorker, &QObject::deleteLater);
    connect(m_scanThread, &QThread::finished, m_scanThread, &QObject::deleteLater);

    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(200);
    connect(m_progressTimer, &QTimer::timeout, this, [this]() {
        if (m_scanWorker) {
            const int progress = m_scanWorker->progressValue();
            if (progress != m_scan_current) {
                m_scan_current = progress;
                emit scanProgressChanged();
            }
        }
    }, Qt::QueuedConnection);
    m_progressTimer->start();

    m_scanning = true;
    emit scanningChanged();

    m_scanThread->start();
    QMetaObject::invokeMethod(m_scanWorker, "process", Qt::QueuedConnection);

    Log.info(QString("Collection import started: %1 (%2)").arg(name, folderPath));
}

void ScanService::finishCollectionImport(int totalAdded, const QList<ScanResultItem>& items)
{
    if (!m_scanning) return;

    if (m_progressTimer) {
        m_progressTimer->stop();
        delete m_progressTimer;
        m_progressTimer = nullptr;
    }

    const QString name = m_pendingCollectionName;
    const QString sourceFolder = m_pendingCollectionPath;
    m_collectionImportActive = false;
    m_pendingCollectionName.clear();
    m_pendingCollectionPath.clear();

    if (!items.isEmpty()) {
        QSet<QString> uniqueCoverHashes;
        for (const auto& item : items) {
            if (!item.coverHash.isEmpty()) {
                uniqueCoverHashes.insert(item.coverHash);
            }
        }

        m_db->beginTransaction();

        for (const QString& hash : uniqueCoverHashes) {
            m_coverManager->persistCoverToDb(hash);
        }

        for (const auto& item : items) {
            MusicItem mi;
            mi.fileHash = item.fileHash;
            mi.filePath = item.filePath;
            mi.title = item.title;
            mi.artist = item.artist;
            mi.album = item.album;
            mi.duration = item.duration;
            mi.coverHash = item.coverHash;
            mi.sectionKey = PinyinUtils::sectionKey(item.title.isEmpty() ? QFileInfo(item.filePath).completeBaseName() : item.title);
            if (!item.coverHash.isEmpty()) {
                mi.coverPath = QUrl::fromLocalFile(m_coverManager->getCoverPath(item.coverHash));
            }

            MusicInfo info;
            info.musicID = item.fileHash;
            info.musicFilePath = item.filePath;
            info.metadata = item.metadata;
            info.coverHash = item.coverHash;
            info.cover = item.cover;
            m_fileManager->trySaveMusicFileInfoToDB(info, mi);
        }

        m_db->commitTransaction();

        int playlistId = m_playlistManager->createCollectionPlaylist(name, sourceFolder);
        if (playlistId > 0) {
            for (const auto& item : items) {
                m_playlistManager->addMusicToPlaylist(playlistId, item.fileHash, false);
            }

            if (m_playlistViewModel) {
                m_playlistViewModel->refresh();
            }
        }

        if (auto* localViewModel = m_vmManager->getViewModel<LocalMusicViewModel>("local_music")) {
            m_fileManager->refreshMusicList();
            auto& refreshedItems = m_fileManager->getMusicList();
            updateCoverPaths(refreshedItems);
            localViewModel->refreshFromDB(QList<MusicItem>(refreshedItems.begin(), refreshedItems.end()));
        }
    }

    m_scanning = false;
    m_scanPhase = 0;
    m_scan_phaseText = "";
    m_scan_currentFile = "";
    m_scan_current = 0;
    m_scan_total = 0;
    emit scanningChanged();
    emit scanPhaseChanged();
    emit scanProgressChanged();
    emit scanCompleted(totalAdded);

    if (m_scanThread) {
        m_scanThread->quit();
    }

    m_scanWorker = nullptr;
    m_scanThread = nullptr;

    Log.info(QString("收藏夹导入完成: %1，共 %2 首歌曲").arg(name).arg(items.size()));
    emit collectionImported();
}

void ScanService::addToLocalMusic(const QString& musicHash)
{
    if (musicHash.isEmpty()) return;

    QVariantMap params;
    params[":is_local"] = 1;
    params[":hash"] = musicHash;
    QString sql = "UPDATE playlist_items SET is_local = :is_local "
                  "WHERE music_hash = :hash "
                  "AND playlist_id IN (SELECT id FROM playlists WHERE is_collection = 1)";
    if (m_db)
        m_db->executeUpdate(sql, params);

    if (auto* localViewModel = m_vmManager->getViewModel<LocalMusicViewModel>("local_music")) {
        MusicItem item = m_fileManager->loadMusicItemByHash(musicHash);
        if (!item.fileHash.isEmpty()) {
            localViewModel->appendItem(item);
        }
    }
}

void ScanService::cancelScan()
{
    if (!m_scanning) return;
    Log.info("Cancelling scan...");
    if (m_scanWorker) {
        m_scanWorker->cancel();
    }
}

QVariantList ScanService::getScanFolders() const
{
    QVariantList list;
#ifdef Q_OS_ANDROID
    for (const auto& sf : AppConfig::instance().scanFolders) {
        QVariantMap m;
        m["path"] = sf.path;
        m["musicCount"] = sf.musicCount;
        m["enabled"] = sf.enabled;
        list.append(m);
    }
#endif
    return list;
}

void ScanService::setScanFolderEnabled(const QString& path, bool enabled)
{
#ifdef Q_OS_ANDROID
    for (auto& sf : AppConfig::instance().scanFolders) {
        if (sf.path == path) { sf.enabled = enabled; break; }
    }
    AppConfig::instance().save();

    if (!enabled && m_fileManager) {
        const auto& musicList = m_fileManager->getMusicList();
        QList<QString> hashesToDelete;
        for (const auto& item : musicList) {
            if (item.filePath.startsWith(path)) {
                hashesToDelete.append(item.fileHash);
            }
        }
        for (const QString& hash : hashesToDelete) {
            m_fileManager->deleteMusicByHash(hash);
            if (m_overrideManager && m_overrideManager->hasOverrides(hash)) {
                m_overrideManager->removeOverride(hash);
            }
        }
        if (m_playlistViewModel) m_playlistViewModel->refresh();
        if (auto* localViewModel = m_vmManager->getViewModel<LocalMusicViewModel>("local_music")) {
            auto& items = m_fileManager->getMusicList();
            localViewModel->refreshFromDB(QList<MusicItem>(items.begin(), items.end()));
        }
    }
#endif
}

void ScanService::clearScanFolders()
{
#ifdef Q_OS_ANDROID
    AppConfig::instance().scanFolders.clear();
    AppConfig::instance().save();
#endif
}

void ScanService::rebuildScanFoldersFromLibrary()
{
#ifdef Q_OS_ANDROID
    if (!m_fileManager) return;

    QMap<QString, bool> savedEnabled;
    for (const auto& sf : AppConfig::instance().scanFolders) {
        savedEnabled[sf.path] = sf.enabled;
    }

    AppConfig::instance().scanFolders.clear();
    const auto& musicList = m_fileManager->getMusicList();

    for (const auto& item : musicList) {
        QString parentDir = QFileInfo(item.filePath).path();
        if (parentDir.isEmpty()) continue;

        bool found = false;
        for (auto& sf : AppConfig::instance().scanFolders) {
            if (sf.path == parentDir) { sf.musicCount++; found = true; break; }
        }
        if (!found) {
            AppConfig::ScanFolderEntry entry;
            entry.path = parentDir;
            entry.musicCount = 1;
            entry.enabled = savedEnabled.value(parentDir, true);
            AppConfig::instance().scanFolders.append(entry);
        }
    }

    for (auto it = savedEnabled.begin(); it != savedEnabled.end(); ++it) {
        if (it.value()) continue;
        bool stillExists = false;
        for (const auto& sf : AppConfig::instance().scanFolders) {
            if (sf.path == it.key()) { stillExists = true; break; }
        }
        if (!stillExists) {
            AppConfig::ScanFolderEntry entry;
            entry.path = it.key();
            entry.musicCount = 0;
            entry.enabled = false;
            AppConfig::instance().scanFolders.append(entry);
        }
    }

    AppConfig::instance().save();
    Log.info(QString("从已有音乐重建文件夹列表: %1 个文件夹").arg(AppConfig::instance().scanFolders.size()));
#endif
}

void ScanService::scanSingleDirectory(const QString& path)
{
    if (m_scanning) return;
    AppConfig::instance().addMusicScanDirectory(path);
    doScanMusicNow();
}

void ScanService::applyMetadataOverrides(QList<MusicItem>& items)
{
    if (!m_overrideManager) return;

    const auto overrides = m_overrideManager->getAllOverrides();
    if (overrides.isEmpty()) return;

    for (auto& item : items) {
        auto it = overrides.find(item.fileHash);
        if (it == overrides.end()) continue;
        const QVariantMap& ov = it.value();
        if (ov.contains("title"))    item.title = ov["title"].toString();
        if (ov.contains("artist"))   item.artist = ov["artist"].toString();
        if (ov.contains("album"))    item.album = ov["album"].toString();
        if (ov.contains("cover_hash") && !ov["cover_hash"].isNull()) {
            item.coverHash = ov["cover_hash"].toString();
            item.coverPath = QUrl::fromLocalFile(
                m_coverManager->getCoverPath(item.coverHash));
        }
    }
}

void ScanService::updateCoverPaths(QList<MusicItem>& musicList)
{
    for (auto& item : musicList) {
        const QString hash = !item.coverHash.isEmpty() ? item.coverHash : item.coverPath.fileName();
        if (!hash.isEmpty()) {
            item.coverPath = QUrl::fromLocalFile(m_coverManager->getCoverPath(hash));
        }
    }
}

#include "scanworker.h"
#include "../filesystem/musicfile/musicscanner.h"
#include "../filesystem/metadata/musiccovermanager.h"
#include "../utils/musicinfo.h"
#include "../utils/music_entity_type.h"
#include "../utils/logger.h"
#include "../threading/threadpoolmanager.h"
#include "appconfig.h"

#include <QFileInfo>
#include <QDir>

// ==================== ScanDirTask (Phase 1, IO_POOL) ====================

ScanDirTask::ScanDirTask(const QString& dirPath, std::atomic<bool>& cancelled)
    : m_dirPath(dirPath)
    , m_cancelled(cancelled)
{
    setAutoDelete(false);
}

void ScanDirTask::run()
{
    if (m_cancelled.load()) return;

    QDir dir(m_dirPath);
    if (!dir.exists()) return;

    QVector<QString> results;
    QVector<QDir> dirStack;
    dirStack.push_back(dir);

    static const QSet<QString> supportedFormats {
        "mp3", "flac", "wav", "ogg", "m4a", "aac", "wma", "ape"
    };

    while (!dirStack.isEmpty() && !m_cancelled.load()) {
        QDir currentDir = dirStack.takeLast();
        QFileInfoList entries = currentDir.entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);

        for (const QFileInfo& entry : entries) {
            if (m_cancelled.load()) return;
            if (entry.isDir()) {
                dirStack.push_back(QDir(entry.absoluteFilePath()));
            } else if (entry.isFile()) {
                QString suffix = entry.suffix().toLower();
                if (supportedFormats.contains(suffix)) {
                    results.append(entry.absoluteFilePath());
                }
            }
        }
    }

    m_results = std::move(results);
}

// ==================== HashBatchTask (Phase 1.5, CPU_POOL) ====================

HashBatchTask::HashBatchTask(const QVector<QString>& filePaths,
                               std::atomic<bool>& cancelled,
                               std::atomic<int>* hashProgress)
    : m_filePaths(filePaths)
    , m_cancelled(cancelled)
    , m_hashProgress(hashProgress)
{
    setAutoDelete(false);
}

void HashBatchTask::run()
{
    m_results.reserve(m_filePaths.size());
    m_failedPaths.reserve(m_filePaths.size());

    for (const QString& path : m_filePaths) {
        if (m_cancelled.load()) return;

        QString id = MusicInfo::generateId(QFileInfo(path));
        if (m_hashProgress) m_hashProgress->fetch_add(1);
        if (!id.isEmpty()) {
            m_results.append(qMakePair(id, path));
        } else {
            m_failedPaths.append(path);
        }
    }
}

// ==================== ScanBatchTask (Phase 2, IO_POOL) ====================

ScanBatchTask::ScanBatchTask(const QVector<QPair<QString, QString>>& files,
                             MusicCoverManager* coverManager,
                             std::atomic<int>& progressCounter,
                             std::atomic<bool>& cancelled)
    : m_files(files)
    , m_coverManager(coverManager)
    , m_progressCounter(progressCounter)
    , m_cancelled(cancelled)
{
    setAutoDelete(false);
}

void ScanBatchTask::run()
{
    m_results.reserve(m_files.size());

    for (const auto& pair : m_files) {
        if (m_cancelled.load()) return;

        try {
            MusicInfo info;
            info.musicID = pair.first;
            info.musicFilePath = pair.second;
            info.extraMusicInfo();

            ScanResultItem item;
            item.fileHash = pair.first;
            item.filePath = pair.second;

            if (!info.metadata.title.isEmpty()) {
                item.title = info.metadata.title;
            } else {
#ifdef Q_OS_ANDROID
                const QString cp = AppConfig::instance().canonicalPath(item.fileHash);
                if (!cp.isEmpty()) {
                    QString name = QFileInfo(cp).completeBaseName();
                    int sep = name.indexOf(QStringLiteral(" - 1."));
                    if (sep > 0) name = name.left(sep);
                    item.title = name;
                } else
#endif
                {
                    item.title = QFileInfo(pair.second).completeBaseName();
                }
            }

            item.artist = info.metadata.artist.isEmpty() ? QStringLiteral("佚名") : info.metadata.artist;
            item.album = info.metadata.album;

            item.duration = formatDuration(info.metadata.lengthInSeconds);
            item.metadata = info.metadata;
            item.cover = info.cover;

            if (!info.cover.isNull() && m_coverManager) {
                item.coverHash = m_coverManager->saveCoverSafe(info.cover);
            }

            m_results.append(item);
            m_progressCounter.fetch_add(1);

        } catch (const std::exception& e) {
            Log.error(QString("ScanBatchTask: %1 - %2").arg(pair.second, e.what()));
            continue;
        }
    }
}

// ==================== ScanWorker ====================

ScanWorker::ScanWorker(const QStringList& scanDirs,
                       MusicScanner* scanner,
                       MusicCoverManager* coverManager,
                       const QSet<QString>& existingIds,
                       int minFileSizeKB,
                       int minDurationSec,
                       QObject* parent)
    : QObject(parent)
    , m_scanDirs(scanDirs)
    , m_scanner(scanner)
    , m_coverManager(coverManager)
    , m_existingIds(existingIds)
    , m_minFileSizeKB(minFileSizeKB)
    , m_minDurationSec(minDurationSec)
{
}

ScanWorker::ScanWorker(const QStringList& preDiscoveredFiles,
                       MusicScanner* scanner,
                       MusicCoverManager* coverManager,
                       const QSet<QString>& existingIds,
                       bool androidSkipPhase1,
                       int minFileSizeKB,
                       int minDurationSec)
    : QObject(nullptr)
    , m_scanner(scanner)
    , m_coverManager(coverManager)
    , m_existingIds(existingIds)
    , m_minFileSizeKB(minFileSizeKB)
    , m_minDurationSec(minDurationSec)
{
    Q_UNUSED(androidSkipPhase1)
    m_allMusicFiles = QVector<QString>(preDiscoveredFiles.begin(), preDiscoveredFiles.end());
}

ScanWorker::~ScanWorker() = default;

void ScanWorker::cancel()
{
    m_cancelled.store(true);
    if (m_scanner) {
        m_scanner->cancelScan();
    }
}

void ScanWorker::process()
{
    m_cancelled.store(false);
    m_progressCounter.store(0);

    QThreadPool* ioPool = ThreadPoolManager::pool(ThreadPoolManager::IO_POOL);

    // ===== Phase 1: 目录扫描 / 安卓用预获取列表 =====
    if (m_allMusicFiles.isEmpty()) {
        emit phaseChanged(1);
        emit phaseTextChanged(QString("正在扫描音乐文件..."));

        Log.info(QString("ScanWorker: 扫描目录列表(共%1): [%2]")
            .arg(m_scanDirs.size()).arg(m_scanDirs.join(", ")));

        if (m_scanDirs.size() <= 1) {
            for (const QString& scanDir : m_scanDirs) {
                if (m_cancelled.load()) {
                    emit errorOccurred("扫描已取消");
                    return;
                }
                const auto& files = m_scanner->scanDirectory(scanDir);
                m_allMusicFiles.append(files);
                Log.info(QString("ScanWorker: 目录 %1 → 发现 %2 个音频文件")
                    .arg(scanDir).arg(files.size()));
                emit progressChanged(m_allMusicFiles.size(), m_allMusicFiles.size(),
                                     QString("已发现 %1 个文件").arg(m_allMusicFiles.size()));
            }
        } else {
            QList<ScanDirTask*> dirTasks;
            dirTasks.reserve(m_scanDirs.size());
            for (const QString& dir : m_scanDirs) {
                auto* task = new ScanDirTask(dir, m_cancelled);
                dirTasks.append(task);
                ioPool->start(task);
            }
            ioPool->waitForDone();

            for (auto* task : dirTasks) {
                m_allMusicFiles.append(task->results());
                delete task;
            }

            emit progressChanged(m_allMusicFiles.size(), m_allMusicFiles.size(),
                                 QString("已发现 %1 个文件").arg(m_allMusicFiles.size()));
        }

        if (m_cancelled.load()) {
            emit errorOccurred("扫描已取消");
            return;
        }
    } else {
        // 安卓路径：文件列表已通过 MediaStore 预获取
        emit phaseChanged(1);
        emit phaseTextChanged(QString("从系统媒体库发现 %1 个文件...").arg(m_allMusicFiles.size()));
        emit progressChanged(m_allMusicFiles.size(), m_allMusicFiles.size(),
                             QString("已发现 %1 个文件").arg(m_allMusicFiles.size()));
    }

    // ── 文件大小过滤 ──
    if (m_minFileSizeKB > 0) {
        QVector<QString> filtered;
        for (const auto& path : m_allMusicFiles) {
            QFileInfo fi(path);
            if (fi.size() >= static_cast<qint64>(m_minFileSizeKB) * 1024) {
                filtered.append(path);
            }
        }
        int removed = m_allMusicFiles.size() - filtered.size();
        if (removed > 0) Log.info(QString("ScanWorker: 跳过 %1 个小于 %2 KB 的文件").arg(removed).arg(m_minFileSizeKB));
        m_allMusicFiles = filtered;
    }

    // ===== Phase 1.5: SHA1 哈希计算 + 去重（CPU_POOL 并行）=====
    emit phaseTextChanged(QString("正在计算文件哈希，已发现 %1 个文件...").arg(m_allMusicFiles.size()));

    QThreadPool* cpuPool = ThreadPoolManager::pool(ThreadPoolManager::CPU_POOL);
    const int hashBatchSize = 100;
    const int totalHashBatches = (m_allMusicFiles.size() + hashBatchSize - 1) / hashBatchSize;
    QList<HashBatchTask*> hashTasks;
    hashTasks.reserve(totalHashBatches);
    std::atomic<int> hashProgress{0};
    const int hashTotal = m_allMusicFiles.size();

    for (int i = 0; i < totalHashBatches; ++i) {
        if (m_cancelled.load()) break;
        int start = i * hashBatchSize;
        int end = qMin(start + hashBatchSize, m_allMusicFiles.size());
        QVector<QString> batch = m_allMusicFiles.mid(start, end - start);
        auto* task = new HashBatchTask(batch, m_cancelled, &hashProgress);
        hashTasks.append(task);
        cpuPool->start(task);
    }

    // 轮询进度
    int lastReported = 0;
    while (cpuPool->activeThreadCount() > 0 || hashProgress.load() < hashTotal) {
        if (m_cancelled.load()) { cpuPool->waitForDone(1000); break; }
        QThread::msleep(500);
        int cur = hashProgress.load();
        if (cur != lastReported && cur > 0 && hashTotal > 0) {
            lastReported = cur;
            m_progressCounter.store(cur);  // 同步统一进度源，防止 timer 覆盖
            emit progressChanged(cur, hashTotal, "");
            emit phaseTextChanged(QString("计算哈希: %1 / %2").arg(cur).arg(hashTotal));
        }
    }
    cpuPool->waitForDone();

    QHash<QString, QString> idPathMap;
    int dedupedCount = 0;
    int hashFailedCount = 0;
    QStringList dedupedSamples, hashFailedSamples;
    for (auto* task : hashTasks) {
        if (m_cancelled.load()) break;
        const auto& taskResults = task->results();
        for (const auto& pair : taskResults) {
            if (!m_existingIds.contains(pair.first)) {
                idPathMap.insert(pair.first, pair.second);
            } else {
                dedupedCount++;
                if (dedupedSamples.size() < 10) {
                    dedupedSamples.append(QString("%1 | %2").arg(
                        QFileInfo(pair.second).fileName(), pair.first.left(12)));
                }
            }
        }
        for (const auto& fp : task->failedPaths()) {
            hashFailedCount++;
            if (hashFailedSamples.size() < 10) {
                hashFailedSamples.append(QFileInfo(fp).fileName());
            }
        }
        delete task;
    }

    if (dedupedCount > 0) {
        Log.info(QString("ScanWorker: existingIds去重 %1 个 (totalHashes=%2) 样本: [%3]")
            .arg(dedupedCount).arg(m_existingIds.size())
            .arg(dedupedSamples.join(", ")));
    }
    if (hashFailedCount > 0) {
        Log.info(QString("ScanWorker: 哈希计算失败 %1 个 样本: [%2]")
            .arg(hashFailedCount).arg(hashFailedSamples.join(", ")));
    }

    if (m_cancelled.load()) {
        emit errorOccurred("扫描已取消");
        return;
    }

    m_newFiles.clear();
    for (const auto& pair : idPathMap.asKeyValueRange()) {
        m_newFiles.append(pair);
    }

    if (m_newFiles.isEmpty()) {
        emit scanFinished(0, {});
        return;
    }

    // ===== Phase 2: 元数据+封面提取（IO_POOL 并行）=====
    // 注意：phase 2 的进度文字由 QML 侧根据 scanCurrent/scanTotal 动态拼装，
    // 这里仅发送 phase 变化和初始 total，不再发送静态的过时文字。
    const int total = m_newFiles.size();
    emit phaseChanged(2);
    emit phaseTextChanged(QString());
    emit progressChanged(0, total, "");

    const int totalBatches = (total + BATCH_SIZE - 1) / BATCH_SIZE;
    QList<ScanBatchTask*> tasks;
    tasks.reserve(totalBatches);

    for (int batchIdx = 0; batchIdx < totalBatches; ++batchIdx) {
        const int start = batchIdx * BATCH_SIZE;
        const int end = qMin(start + BATCH_SIZE, total);
        QVector<QPair<QString, QString>> batch = m_newFiles.mid(start, end - start);

        auto* task = new ScanBatchTask(batch, m_coverManager, m_progressCounter, m_cancelled);
        tasks.append(task);
        ioPool->start(task);
    }

    ioPool->waitForDone();

    QList<ScanResultItem> allResults;
    for (auto* task : tasks) {
        allResults.append(task->results());
        delete task;
    }

    if (m_cancelled.load()) {
        emit scanFinished(allResults.size(), allResults);
        return;
    }

    // ── 时长过滤 ──
    if (m_minDurationSec > 0) {
        QList<ScanResultItem> filtered;
        for (const auto& item : allResults) {
            if (item.metadata.lengthInSeconds >= m_minDurationSec) {
                filtered.append(item);
            }
        }
        int removed = allResults.size() - filtered.size();
        if (removed > 0) Log.info(QString("ScanWorker: 跳过 %1 个小于 %2 秒的文件").arg(removed).arg(m_minDurationSec));
        allResults = filtered;
    }

    emit progressChanged(total, total, "");
    emit phaseTextChanged(QString("扫描完成"));
    emit scanFinished(allResults.size(), allResults);
}

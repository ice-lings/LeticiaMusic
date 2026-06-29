#ifndef SCANWORKER_H
#define SCANWORKER_H

#include <QObject>
#include <QThread>
#include <QRunnable>
#include <QVector>
#include <QPair>
#include <QMutex>
#include <QSet>
#include <QImage>
#include <atomic>

#include "../utils/music_entity_type.h"

class MusicScanner;
class MusicCoverManager;

struct ScanResultItem {
    QString fileHash;
    QString filePath;
    QString title;
    QString artist;
    QString album;
    QString duration;
    QString coverHash;
    QImage cover;
    MusicMetadata metadata;
};

class ScanDirTask : public QRunnable
{
public:

    explicit ScanDirTask(const QString& dirPath, std::atomic<bool>& cancelled);
    void run() override;
    const QVector<QString>& results() const { return m_results; }
private:
    QString m_dirPath;
    QVector<QString> m_results;
    std::atomic<bool>& m_cancelled;
};

class HashBatchTask : public QRunnable
{
public:
    explicit HashBatchTask(const QVector<QString>& filePaths,
                           std::atomic<bool>& cancelled,
                           std::atomic<int>* hashProgress = nullptr);
    void run() override;
    const QVector<QPair<QString, QString>>& results() const { return m_results; }
    const QVector<QString>& failedPaths() const { return m_failedPaths; }
private:
    QVector<QString> m_filePaths;
    QVector<QPair<QString, QString>> m_results;
    QVector<QString> m_failedPaths;
    std::atomic<bool>& m_cancelled;
    std::atomic<int>* m_hashProgress = nullptr;
};

class ScanBatchTask : public QRunnable
{
public:
    explicit ScanBatchTask(const QVector<QPair<QString, QString>>& files,
                           MusicCoverManager* coverManager,
                           std::atomic<int>& progressCounter,
                           std::atomic<bool>& cancelled);
    void run() override;
    const QList<ScanResultItem>& results() const { return m_results; }

private:
    QVector<QPair<QString, QString>> m_files;
    MusicCoverManager* m_coverManager = nullptr;
    QList<ScanResultItem> m_results;
    std::atomic<int>& m_progressCounter;
    std::atomic<bool>& m_cancelled;
};

class ScanWorker : public QObject
{
    Q_OBJECT
public:
    explicit ScanWorker(const QStringList& scanDirs,
                        MusicScanner* scanner,
                        MusicCoverManager* coverManager,
                        const QSet<QString>& existingIds,
                        int minFileSizeKB = 0,
                        int minDurationSec = 0,
                        QObject* parent = nullptr);

    /* 安卓专用：直接使用预发现文件列表，跳过 Phase 1 目录扫描 */
    explicit ScanWorker(const QStringList& preDiscoveredFiles,
                        MusicScanner* scanner,
                        MusicCoverManager* coverManager,
                        const QSet<QString>& existingIds,
                        bool androidSkipPhase1,
                        int minFileSizeKB = 0,
                        int minDurationSec = 0);

    ~ScanWorker() override;

    Q_INVOKABLE void cancel();
    Q_INVOKABLE void process();

    int progressValue() const { return m_progressCounter.load(); }

signals:
    void phaseChanged(int phase);
    void progressChanged(int current, int total, const QString& currentFile);
    void phaseTextChanged(const QString& text);
    void scanFinished(int totalAdded, const QList<ScanResultItem>& items);
    void errorOccurred(const QString& message);

private:
    static constexpr int BATCH_SIZE = 50;

    QStringList m_scanDirs;
    MusicScanner* m_scanner = nullptr;
    MusicCoverManager* m_coverManager = nullptr;
    QSet<QString> m_existingIds;

    QVector<QString> m_allMusicFiles;
    QVector<QPair<QString, QString>> m_newFiles;
    std::atomic<bool> m_cancelled{false};
    std::atomic<int> m_progressCounter{0};
    int m_minFileSizeKB = 0;
    int m_minDurationSec = 0;
};

#endif // SCANWORKER_H
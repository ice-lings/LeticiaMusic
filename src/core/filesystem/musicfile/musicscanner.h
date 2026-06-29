#ifndef MUSICSCANNER_H
#define MUSICSCANNER_H

#include <QObject>
#include <QSet>
#include <QRunnable>
#include <QQueue>
#include <QWaitCondition>
#include <QMutex>
#include <QSharedPointer>
#include <QFile>

#include "../../utils/music_entity_type.h"

class MusicScanner : public QObject
{
    Q_OBJECT
public:
    explicit MusicScanner(QObject *parent = nullptr);
    ~MusicScanner() override = default;

    MusicScanner(const MusicScanner&) = delete;
    MusicScanner& operator=(const MusicScanner&) = delete;

    QVector<QString> scanDirectory(const QString &directoryPath, bool includeSubdirs = true);
    QVector<QString> processBatch(const QStringList& batchPaths);
    void cancelScan();

signals:

    void filesFound(const MusicFileRefVector& files);
    void scanFinished();

private:
    inline static constexpr int BATCH_SIZE = 50;
    inline static constexpr int COVER_BATCH_SIZE = 25;

    inline static const QSet<QString> m_supportedFormats {
        "mp3", "flac", "wav", "ogg", "m4a", "aac", "wma", "ape"
    };

    class ExtractMetadata : public QRunnable {
    public:
        ExtractMetadata(MusicScanner* scanner, const QStringList& paths)
            : m_scanner(scanner), m_musicPaths(paths) {
            setAutoDelete(true);
        }

        void run() override;
        QStringList m_musicPaths;
        MusicScanner* m_scanner;
    };

    class ExtractCover : public QRunnable {
    public:
        ExtractCover(MusicScanner* scanner, const QStringList& paths)
            : m_scanner(scanner), m_musicPaths(paths) {
            setAutoDelete(true);
        }
        void run() override;
        QStringList m_musicPaths;
        MusicScanner* m_scanner;
    };

    void enqueueTask(const QStringList& musicFilePaths);
    void enqueueTask(const QString &directoryPath, bool includeSubdirs);
    void startNextTask();
    Q_INVOKABLE void processTaskCompletion();

    QQueue<ExtractMetadata*> m_taskQueue;
    QMutex m_queueMutex;

    std::atomic<bool> m_cancelled{false};
    std::atomic<int> m_activeTasks{0};

    // 安全的任务清理辅助方法
    void cleanupPendingTasks();

};


#endif // MUSICSCANNER_H

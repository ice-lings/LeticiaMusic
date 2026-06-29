#include "musicscanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>
#include <memory>

#include "../../threading/threadpoolmanager.h"

#include "../../utils/musicinfo.h"
#include "../../utils/logger.h"



MusicScanner::MusicScanner(QObject *parent)
    : QObject{parent}
{

}

QVector<QString> MusicScanner::scanDirectory(const QString &directoryPath, bool includeSubdirs)
{
    m_cancelled = false;
    QVector<QString> MusicFilePathVec;

    QDir dir(directoryPath);
    if (!dir.exists()) return MusicFilePathVec;

    QVector<QDir> dirStack;
    dirStack.reserve(64);
    dirStack.push_back(dir);

    while (!dirStack.isEmpty() && !m_cancelled) {
        QDir currentDir = dirStack.takeLast();

        QFileInfoList entries = currentDir.entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable
            );

        for (const QFileInfo& entry : entries) {
            if (m_cancelled) return MusicFilePathVec;

            if (entry.isDir() && includeSubdirs) {
                dirStack.push_back(QDir(entry.absoluteFilePath()));
            }
            else if (entry.isFile()) {
                QString suffix = entry.suffix().toLower();
                if (m_supportedFormats.contains(suffix)) {
                    MusicFilePathVec.append(entry.absoluteFilePath());
                }
            }
        }
    }

    return MusicFilePathVec;
}

QVector<QString> MusicScanner::processBatch(const QStringList &batchPaths)
{
    QVector<QString> batchIds;
    batchIds.reserve(batchPaths.size()); // 预分配100个空间

    // 批量生成ID
    for (const QString& path : batchPaths) {
        QString id = MusicInfo::generateId(QFileInfo(path));
        if (!id.isEmpty()) { // 过滤无效ID（文件不存在等）
            batchIds.push_back(id);
        }
    }

    return batchIds;
}

void MusicScanner::cleanupPendingTasks()
{
    QMutexLocker locker(&m_queueMutex);
    // 只清理等待队列中的任务，不删除正在执行的任务
    while (!m_taskQueue.isEmpty()) {
        ExtractMetadata* task = m_taskQueue.dequeue();
        // 标记任务为已取消，让任务在运行时自行清理
        delete task;
    }
}

void MusicScanner::cancelScan()
{
    m_cancelled = true;

    // 安全地清理等待队列，不删除正在执行的任务
    cleanupPendingTasks();

    // 等待正在执行的任务完成（可选）
    // 这里不强制删除正在运行的任务，让它们自然完成
}


void MusicScanner::enqueueTask(const QStringList &musicFilePaths)
{
    QMutexLocker locker(&m_queueMutex);
    auto task = std::make_unique<ExtractMetadata>(this, musicFilePaths);
    task->setAutoDelete(true);
    m_taskQueue.enqueue(task.release());
}


void MusicScanner::startNextTask()
{
    QMutexLocker locker(&m_queueMutex);

    if (m_taskQueue.isEmpty()) return;

    ExtractMetadata* task = m_taskQueue.dequeue();
    m_activeTasks++;

    ThreadPoolManager::pool(ThreadPoolManager::IO_POOL)->start(task);
}


void MusicScanner::processTaskCompletion()
{
    m_activeTasks--;

    // 检查是否有新任务
    startNextTask();

    // 所有任务完成
    if (m_activeTasks == 0 && m_taskQueue.isEmpty()) {
        emit scanFinished();
    }
}


void MusicScanner::ExtractMetadata::run()
{
    if (m_scanner->m_cancelled) return;

    MusicFileRefVector foundFiles;
    foundFiles.reserve(m_musicPaths.size());

    for (const auto& path : m_musicPaths) {
        if (m_scanner->m_cancelled) return;

    }
    Log.debug("MusicScanner has sended data: " + QString::number(foundFiles.size()));
    emit m_scanner->filesFound(foundFiles);
    QMetaObject::invokeMethod(m_scanner, "processTaskCompletion", Qt::QueuedConnection);
}

void MusicScanner::ExtractCover::run()
{
    if (m_scanner->m_cancelled) return;


}

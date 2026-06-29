#include "ftp_thread_pool.h"
#include "../utils/logger.h"

void FtpWorkerThread::run()
{
    Log.debug(QString("[FtpThreadPool][Worker-%1] 线程启动 tid=0x%2")
        .arg(m_id).arg((quintptr)QThread::currentThreadId(), 0, 16));

    FtpSession session(m_config);
    session.setCache(&m_ctx.dirCache);

    while (true) {
        FtpTask task;
        {
            std::unique_lock<std::mutex> lock(m_ctx.queueMutex);
            m_ctx.queueCond.wait(lock, [this] {
                return !m_ctx.taskQueue.empty() || m_ctx.shutdown
                    || m_ctx.cancelled.load(std::memory_order_relaxed);
            });
            if (m_ctx.cancelled.load(std::memory_order_relaxed)) break;   // 取消：丢弃剩余
            if (m_ctx.shutdown && m_ctx.taskQueue.empty()) break;          // 优雅停止
            task = m_ctx.taskQueue.front();
            m_ctx.taskQueue.pop();
        }

        FtpSession::TransferResult result =
            task.isUpload
                ? session.uploadFile(task.localPath, task.remotePath)
                : session.downloadFile(task.remotePath, task.localPath);

        switch (result) {
            case FtpSession::TransferResult::Success:
                m_ctx.successCount.fetch_add(1, std::memory_order_relaxed);
                break;
            case FtpSession::TransferResult::Skipped:
                m_ctx.skippedCount.fetch_add(1, std::memory_order_relaxed);
                break;
            case FtpSession::TransferResult::Failed:
                m_ctx.failCount.fetch_add(1, std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> lock(m_ctx.failMutex);
                    m_ctx.failedFiles.append(task.canonicalPath + " (" + session.lastError() + ")");
                }
                break;
        }
    }

    Log.debug(QString("[FtpThreadPool][Worker-%1] 线程退出").arg(m_id));
    // session 在此析构，RAII 释放私有句柄
}

void FtpThreadPool::start()
{
    m_context.shutdown = false;
    m_context.cancelled.store(false, std::memory_order_relaxed);
    m_workers.reserve(m_threadCount);
    for (int i = 0; i < m_threadCount; i++) {
        m_workers.emplace_back(std::make_unique<FtpWorkerThread>(i, m_config, m_context));
        m_workers.back()->start();
    }
}

void FtpThreadPool::prewarmDirs(const QStringList& remoteDirs)
{
    if (remoteDirs.isEmpty()) return;
    FtpSession session(m_config);
    session.setCache(&m_context.dirCache);
    for (const QString& dir : remoteDirs) {
        if (!dir.isEmpty()) session.ensureRemoteDir(dir);
    }
}

void FtpThreadPool::submitTasks(const QList<FtpTask>& tasks)
{
    m_totalTasks += tasks.size();
    {
        std::lock_guard<std::mutex> lock(m_context.queueMutex);
        for (const auto& t : tasks) m_context.taskQueue.push(t);
    }
    m_context.queueCond.notify_all();
}

void FtpThreadPool::waitForAll()
{
    {
        std::lock_guard<std::mutex> lock(m_context.queueMutex);
        m_context.shutdown = true;
    }
    m_context.queueCond.notify_all();
    for (auto& w : m_workers) w->wait();   // 先全部 join
    m_workers.clear();                      // 再销毁（unique_ptr delete）
}

void FtpThreadPool::requestCancel()
{
    m_context.cancelled.store(true, std::memory_order_relaxed);
    m_context.queueCond.notify_all();
}

QStringList FtpThreadPool::failedFiles()
{
    std::lock_guard<std::mutex> lock(m_context.failMutex);
    return m_context.failedFiles;
}

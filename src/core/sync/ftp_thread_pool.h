#ifndef FTP_THREAD_POOL_H
#define FTP_THREAD_POOL_H

#include <QThread>
#include <QString>
#include <QStringList>
#include <QList>

#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>

#include "ftp_session.h"

// 单个传输任务（上传或下载）
struct FtpTask
{
    bool    isUpload = true;     // true=上传, false=下载
    QString localPath;           // 上传源 / 下载目标
    QString remotePath;
    QString canonicalPath;       // 失败列表 key
};

// 所有 worker 共享的池状态（不可拷贝：含 mutex/atomic）
struct PoolContext
{
    std::queue<FtpTask>      taskQueue;
    std::mutex              queueMutex;
    std::condition_variable queueCond;
    bool                    shutdown = false;     // 优雅停止（受 queueMutex 保护）
    std::atomic<bool>       cancelled{false};     // 取消（锁外快查）
    DirExistsCache          dirCache;             // 共享目录缓存

    std::atomic<int>        successCount{0};
    std::atomic<int>        failCount{0};
    std::atomic<int>        skippedCount{0};

    std::mutex              failMutex;
    QStringList             failedFiles;          // 条目: canonicalPath + " (原因)"
};

class FtpWorkerThread : public QThread
{
public:
    FtpWorkerThread(int id, const FtpConfig& cfg, PoolContext& ctx)
        : m_id(id), m_config(cfg), m_ctx(ctx) {}
    void run() override;

private:
    int          m_id = 0;
    FtpConfig    m_config;
    PoolContext& m_ctx;
};

class FtpThreadPool
{
public:
    FtpThreadPool(const FtpConfig& cfg, int threadCount = 4)
        : m_config(cfg), m_threadCount(threadCount) {}

    void start();
    void prewarmDirs(const QStringList& remoteDirs);
    void submitTasks(const QList<FtpTask>& tasks);
    void waitForAll();
    void requestCancel();

    int successCount() const { return m_context.successCount.load(std::memory_order_relaxed); }
    int failCount()    const { return m_context.failCount.load(std::memory_order_relaxed); }
    int skippedCount() const { return m_context.skippedCount.load(std::memory_order_relaxed); }
    int processedCount() const { return successCount() + failCount() + skippedCount(); }
    int totalTasks()   const { return m_totalTasks; }
    QStringList failedFiles();   // 加锁拷贝返回

private:
    FtpConfig   m_config;
    int         m_threadCount = 4;
    int         m_totalTasks = 0;
    PoolContext m_context;
    std::vector<std::unique_ptr<FtpWorkerThread>> m_workers;
};

#endif // FTP_THREAD_POOL_H

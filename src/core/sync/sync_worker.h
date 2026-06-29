#ifndef SYNC_WORKER_H
#define SYNC_WORKER_H

#include <QObject>
#include <QThread>
#include <QAtomicInt>
#include <QMap>
#include <QPair>
#include <QSet>
#include <optional>
#include <memory>
#include "sync_manifest.h"
#include "sync_config.h"
#include "ftp_thread_pool.h"



// 后台线程同步工作者 — 支持并发多连接 FTP 传输
// 在主线程构建本地清单后, 将 FTP 连接/远程清单/对比/传输/数据同步 移至后台线程
class SyncWorker : public QObject
{
    Q_OBJECT

public:
    explicit SyncWorker(QObject* parent = nullptr);
    ~SyncWorker() override;

    // 设置参数 (主线程调用, 在 start() 之前)
    void setFtpConfig(const FtpConfig& cfg);
    void setSyncConfig(const SyncConfig& cfg);
    void setLocalManifest(const SyncManifest& manifest);
    void setConfirmedDeletions(const QList<DeletedFileLogEntry>& deletions);
    void setLocalPathMap(const QMap<QString, QString>& hashToPath);
    void setLocalSyncDataJson(const QByteArray& json);
    void setCoordinator(class SyncCoordinator* c) { m_coordinator = c; }
    QByteArray mergedSyncData() const { return m_mergedSyncData; }
    SyncManifest mergedManifest() const { return m_mergedManifest; }
    QList<QPair<QString, QString>> downloadedFiles() const { return m_downloadedFiles; }

    // 并发连接数 (默认 4)
    void setConcurrency(int n) { m_concurrency = n; }
    void setKnownLocalHashes(const QSet<QString>& hashes) { m_knownLocalHashes = hashes; }

    SyncPlan compareManifests(const SyncManifest& remoteManifest,
                               const QSet<QString>& mergedDeletedHashes);

public slots:
    void doSync();
    void cancel();              // 主线程调用, 请求取消

signals:
    void phaseChanged(int phase, const QString& phaseName);
    void progressChanged(int current, int total, const QString& currentFile);
    void finished(int totalOps, int conflictsResolved, int failedCount, const QStringList& failedFiles);
    void errorOccurred(const QString& message);

private:
    SyncManifest fetchRemoteManifest(FtpSession& ftp);
    QByteArray   fetchRemoteSyncData(FtpSession& ftp);
    bool executeFileTransfers(SyncPlan& plan, FtpSession& ftp);
    bool syncData(FtpSession& ftp);
    void finalizeSync(bool success);


    // 配置
    FtpConfig m_ftpCfg;
    SyncConfig m_syncCfg;
    SyncManifest m_localManifest;
    SyncManifest m_remoteManifest;   // 远程清单 (合并用)
    QList<DeletedFileLogEntry> m_confirmedDeletions;
    QMap<QString, QString> m_hashToPath;
    QByteArray m_localSyncDataJson;
    QByteArray m_mergedSyncData;
    SyncManifest m_mergedManifest;
    QList<QPair<QString, QString>> m_downloadedFiles;
    QSet<QString> m_knownLocalHashes;       // 本地所有已知 file_hash（用于下载前去重）
    std::optional<bool> m_isIncremental;      // 全量/增量模式缓存，nullopt 时需 fileExists 获取
    SyncCoordinator* m_coordinator = nullptr;

    // 并发传输控制
    int m_concurrency = 1;
    QAtomicInt m_taskCancelled;
    QStringList m_taskFailedFiles;
    int m_taskTotal = 0;
};

#endif // SYNC_WORKER_H

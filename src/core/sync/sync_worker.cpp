#include "sync_worker.h"
#include "sync_config.h"
#include "sync_coordinator.h"
#include "../utils/logger.h"
#include "../utils/platform_path_helper.h"
#include "../threading/threadpoolmanager.h"
#include "../application/appconfig.h"

#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QThread>
#include <QThreadPool>
#include <memory>

#ifdef Q_OS_WIN
#include <sys/utime.h>
#else
#include <utime.h>
#endif

SyncWorker::SyncWorker(QObject* parent)
    : QObject(parent)
{
}

SyncWorker::~SyncWorker()
{

}

void SyncWorker::setFtpConfig(const FtpConfig& cfg) { m_ftpCfg = cfg; }
void SyncWorker::setSyncConfig(const SyncConfig& cfg) { m_syncCfg = cfg; }
void SyncWorker::setLocalManifest(const SyncManifest& m) { m_localManifest = m; }
void SyncWorker::setConfirmedDeletions(const QList<DeletedFileLogEntry>& d) { m_confirmedDeletions = d; }
void SyncWorker::setLocalPathMap(const QMap<QString, QString>& m) { m_hashToPath = m; }
void SyncWorker::setLocalSyncDataJson(const QByteArray& json) { m_localSyncDataJson = json; }

// ═══════════════════ 取消 ═══════════════════

void SyncWorker::cancel()
{
    m_taskCancelled.storeRelaxed(1);
}

// ═══════════════════ 主入口 (后台线程执行) ═══════════════════

void SyncWorker::doSync()
{
    m_taskCancelled.storeRelaxed(0);

    struct IncrementalGuard {
        std::optional<bool>& opt;
        ~IncrementalGuard() { opt = std::nullopt; }
    } incGuard {m_isIncremental};


    // Phase 1-2: 连接 NAS
    emit phaseChanged(1, "连接 NAS");
    FtpSession session(m_ftpCfg);

    // Phase 3: 获取远程清单（缓存 fileExists 结果，避免重复查询）
    emit phaseChanged(3, "获取远程清单");
    QString manifestPath = session.makeRemotePath("manifest.json");
    if (!m_isIncremental.has_value()) {
        m_isIncremental = session.fileExists(manifestPath);
    }
    bool isIncremental = m_isIncremental.value();

    Log.debug(QString("[SyncWorker] 远程 manifest.json %1: %2")
        .arg(isIncremental ? "存在" : "不存在")
        .arg(manifestPath));

    QSet<QString> mergedDeletedHashes;
    SyncPlan plan;

    if (isIncremental) {
        Log.info("[SyncWorker] 增量同步模式");
        m_remoteManifest = fetchRemoteManifest(session);

        QByteArray remoteSyncData = fetchRemoteSyncData(session);
        if (!remoteSyncData.isEmpty()) {
            QJsonObject remoteObj = QJsonDocument::fromJson(remoteSyncData).object();
            QJsonArray remoteDel = remoteObj.value("deleted_entries").toArray();
            for (const auto& v : remoteDel) {
                QString h = v.toObject()["hash"].toString();
                if (!h.isEmpty()) mergedDeletedHashes.insert(h);
            }
        }
        for (const auto& entry : m_confirmedDeletions) {
            mergedDeletedHashes.insert(entry.fileHash);
        }
    } else {
        Log.info("[SyncWorker] 全量同步模式（远端无清单）");
    }

    // Phase 4: 对比
    Log.info(QString("[SyncWorker] 已合并 %1 条删除记录").arg(mergedDeletedHashes.size()));
    emit phaseChanged(4, "对比差异");
    plan = compareManifests(m_remoteManifest, mergedDeletedHashes);
    Log.info(QString("[SyncWorker] 对比结果: 上传=%1 下载=%2 删除=%3 冲突=%4")
    .arg(plan.filesToUpload.size()).arg(plan.filesToDownload.size())
    .arg(plan.filesToDelete.size()).arg(plan.conflictsResolved));

    // Phase 5: 传输文件
    if (!plan.isEmpty()) {
        emit phaseChanged(5, "传输文件");
        Log.info(QString("[SyncWorker] 开始传输 (%1 项, %2 并发)...")
            .arg(plan.totalOps()).arg(m_concurrency));
        executeFileTransfers(plan, session);
    }

    // Phase 6: 数据同步
    if (!m_taskCancelled.loadRelaxed()) {
        emit phaseChanged(6, "同步数据");
        syncData(session);
    }

    // Phase 7: 合并并上传 manifest
    if (!m_taskCancelled.loadRelaxed()) {
        emit phaseChanged(7, "上传清单");
        SyncManifest merged = m_remoteManifest;

        for (const QString& hash : mergedDeletedHashes) {
            merged.files.remove(hash);
        }

        merged.lastSync = QDateTime::currentSecsSinceEpoch();

        for (auto it = m_localManifest.files.constBegin(); it != m_localManifest.files.constEnd(); ++it) {
            merged.files[it.key()] = it.value();
        }
        for (auto it = m_localManifest.covers.constBegin(); it != m_localManifest.covers.constEnd(); ++it) {
            merged.covers[it.key()] = it.value();
        }
        for (auto it = m_localManifest.musicMetadata.constBegin(); it != m_localManifest.musicMetadata.constEnd(); ++it) {
            merged.musicMetadata[it.key()] = it.value();
        }

        m_mergedManifest = merged;

        Log.info(QString("[SyncWorker] 合并清单: 文件=%1 (本地%2+远程%3) 封面=%4 (本地%5+远程%6)")
            .arg(merged.files.size())
            .arg(m_localManifest.files.size())
            .arg(m_remoteManifest.files.size())
            .arg(merged.covers.size())
            .arg(m_localManifest.covers.size())
            .arg(m_remoteManifest.covers.size()));

        QJsonDocument doc(merged.toJson());
        QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
        if (!session.uploadBytes(manifestPath, jsonData)) {
            Log.warning("[SyncWorker] 上传 manifest.json 失败");
        } else {
            Log.info("[SyncWorker] 合并清单已上传");
        }
    } else {
        Log.info("[SyncWorker] 同步已取消/中断, 跳过数据同步和清单上传");
    }

    emit finished(plan.totalOps(), plan.conflictsResolved,
                  plan.failedFiles.size(), plan.failedFiles);
}

// ═══════════════════ Phase: 远程清单 ═══════════════════

SyncManifest SyncWorker::fetchRemoteManifest(FtpSession& FtpSession)
{
    QString manifestPath = FtpSession.makeRemotePath("manifest.json");
    QByteArray data = FtpSession.downloadBytes(manifestPath);
    
    if (data.isEmpty()) {
        Log.info(QString("[SyncWorker] 远程 manifest.json 为空或获取失败: %1 (%2)")
            .arg(manifestPath, FtpSession.lastError()));
        return SyncManifest();
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        Log.warning("[SyncWorker] 远程 manifest.json 格式无效");
        return SyncManifest();
    }

    SyncManifest manifest = SyncManifest::fromJson(doc.object());
    Log.info(QString("[SyncWorker] 远程清单读取成功: 版本=%1 设备=%2 文件=%3 封面=%4")
        .arg(manifest.version)
        .arg(manifest.deviceId)
        .arg(manifest.files.size())
        .arg(manifest.covers.size()));
    return manifest;
}

QByteArray SyncWorker::fetchRemoteSyncData(FtpSession& FtpSession)
{
    return FtpSession.downloadBytes(FtpSession.makeRemotePath("sync_data.json"));
}

// ═══════════════════ Phase: 对比 ═══════════════════

SyncPlan SyncWorker::compareManifests(const SyncManifest& remoteManifest,
                                      const QSet<QString>& mergedDeletedHashes)
{
    SyncPlan plan;
    const SyncManifest& remote = remoteManifest;

    QSet<QString> allHashes;
    for (auto it = m_localManifest.files.constBegin(); it != m_localManifest.files.constEnd(); ++it)
        allHashes.insert(it.key());
    for (auto it = remote.files.constBegin(); it != remote.files.constEnd(); ++it)
        allHashes.insert(it.key());

    for (const QString& hash : allHashes) {
        if (mergedDeletedHashes.contains(hash)) {
            if (remote.files.contains(hash)) {
                SyncPlan::FileOp op;
                op.hash = hash;
                op.canonicalPath = remote.files[hash].relativePath;
                plan.filesToDelete.append(op);
            }
            continue;
        }

        bool inLocal = m_localManifest.files.contains(hash);
        bool inRemote = remote.files.contains(hash);

        if (inLocal && !inRemote) {
            SyncPlan::FileOp op;
            op.hash = hash;
            op.canonicalPath = m_localManifest.files[hash].relativePath;
            op.size = m_localManifest.files[hash].size;
            op.modified = m_localManifest.files[hash].modified;
            plan.filesToUpload.append(op);
        }
        else if (!inLocal && inRemote) {
            SyncPlan::FileOp op;
            op.hash = hash;
            op.canonicalPath = remote.files[hash].relativePath;
            op.size = remote.files[hash].size;
            op.modified = remote.files[hash].modified;
            plan.filesToDownload.append(op);
        }
        else if (inLocal && inRemote) {
            // 两端都有 — 比较 modified 时间戳决定是否更新
            qint64 localMod = m_localManifest.files[hash].modified;
            qint64 remoteMod = remote.files[hash].modified;
            if (remoteMod > localMod) {
                SyncPlan::FileOp op;
                op.hash = hash;
                op.canonicalPath = remote.files[hash].relativePath;
                op.size = remote.files[hash].size;
                op.modified = remoteMod;
                plan.filesToDownload.append(op);
            } else if (localMod > remoteMod) {
                SyncPlan::FileOp op;
                op.hash = hash;
                op.canonicalPath = m_localManifest.files[hash].relativePath;
                op.size = m_localManifest.files[hash].size;
                op.modified = localMod;
                plan.filesToUpload.append(op);
            }
        }
    }

    // 封面对比
    for (auto it = m_localManifest.covers.constBegin(); it != m_localManifest.covers.constEnd(); ++it) {
        if (!remote.covers.contains(it.key())) {
            SyncPlan::FileOp op;
            op.hash = it.key();
            op.isCover = true;
            op.canonicalPath = QString("covers/%1/%2/%3.jpg").arg(it.key().left(2), it.key().mid(2, 2), it.key());
            op.size = it.value().size;
            op.modified = it.value().modified;
            plan.coversToUpload.append(op);
        }
    }
    for (auto it = remote.covers.constBegin(); it != remote.covers.constEnd(); ++it) {
        if (!m_localManifest.covers.contains(it.key())) {
            SyncPlan::FileOp op;
            op.hash = it.key();
            op.isCover = true;
            op.canonicalPath = QString("covers/%1/%2/%3.jpg").arg(it.key().left(2), it.key().mid(2, 2), it.key());
            op.size = it.value().size;
            op.modified = it.value().modified;
            plan.coversToDownload.append(op);
        }
    }

    return plan;
}

// ═══════════════════ Phase: 并发文件传输 ═══════════════════

static constexpr int MAX_LOCAL_PATH_UTF8 = 250;
static const QRegularExpression s_dedupRe(QStringLiteral(" [-_](?:\\d+|副本|Copy|copy)\\."));

static QString sanitizeLongFileName(const QString& fileName, const QString& hash4, int maxNameUtf8)
{
    QString result = fileName;

    QRegularExpressionMatch match = s_dedupRe.match(result);
    if (match.hasMatch() && match.capturedStart() > 0) {
        int dot = result.lastIndexOf('.');
        if (dot > match.capturedStart())
            result = result.left(match.capturedStart()) + result.mid(dot);
    }

    if (result.toUtf8().size() > maxNameUtf8) {
        int dot = result.lastIndexOf('.');
        QString base = (dot > 0) ? result.left(dot) : result;
        QString ext  = (dot > 0) ? result.mid(dot) : QString();
        int reserved = hash4.size() + 1 + ext.toUtf8().size();
        while (!base.isEmpty() && (base.toUtf8().size() + reserved) > maxNameUtf8)
            base.chop(1);
        result = base + QStringLiteral("_") + hash4 + ext;
    }
    return result;
}

bool SyncWorker::executeFileTransfers(SyncPlan& plan, FtpSession& ftp)
{
    m_taskTotal = plan.totalOps();
    m_taskFailedFiles.clear();

    QList<FtpTask> tasks;
    // 上传任务
    for (const auto& op : (plan.filesToUpload + plan.coversToUpload)) {
        FtpTask t; t.isUpload = true;
        t.localPath = op.isCover
            ? PlatformPathHelper::appDataDir() + "/covers/" + op.hash.left(2) + "/" + op.hash.mid(2,2) + "/" + op.hash + ".jpg"
            : m_hashToPath.value(op.hash);
        t.remotePath = ftp.makeRemotePath(op.canonicalPath);
        t.canonicalPath = op.canonicalPath;
        tasks.append(t);
    }
    // 下载任务（含 m_knownLocalHashes 去重；全量上传场景为空）
    for (const auto& op : (plan.filesToDownload + plan.coversToDownload)) {
        QString rel = op.canonicalPath; if (rel.startsWith("music/")) rel = rel.mid(6);
        if (m_knownLocalHashes.contains(op.hash)) {
            m_downloadedFiles.append({op.hash, PlatformPathHelper::downloadDir() + "/" + rel});
            continue;
        }
        FtpTask t; t.isUpload = false;
        t.remotePath = ftp.makeRemotePath(op.canonicalPath);
        QString localPath = op.isCover
            ? PlatformPathHelper::appDataDir() + "/covers/" + op.hash.left(2) + "/" + op.hash.mid(2,2) + "/" + op.hash + ".jpg"
            : PlatformPathHelper::downloadDir() + "/" + rel;
#ifdef Q_OS_ANDROID
        if (!op.isCover && localPath.toUtf8().size() > MAX_LOCAL_PATH_UTF8) {
            QString origName = QFileInfo(localPath).fileName();
            int dirUtf8 = QFileInfo(localPath).absolutePath().toUtf8().size();
            int maxNameUtf8 = MAX_LOCAL_PATH_UTF8 - dirUtf8 - 1;
            QString safeName = sanitizeLongFileName(origName, op.hash.left(4), maxNameUtf8);
            localPath = QFileInfo(localPath).absolutePath() + "/" + safeName;
            AppConfig::instance().setCanonicalPathEntry(op.hash, op.canonicalPath);
            Log.warning(QString("[SyncWorker] 长文件名缩短: %1 → %2 (总%3B)")
                .arg(origName).arg(safeName).arg(localPath.toUtf8().size()));
        }
#endif
        t.localPath = localPath;
        t.canonicalPath = op.canonicalPath;
        tasks.append(t);
    }

    FtpThreadPool pool(m_ftpCfg, m_concurrency);
    pool.start();
    pool.prewarmDirs({ ftp.makeRemotePath("music"), ftp.makeRemotePath("covers") });
    pool.submitTasks(tasks);
    while (pool.processedCount() < pool.totalTasks()) {
        if (m_taskCancelled.loadRelaxed()) { pool.requestCancel(); break; }
        QThread::msleep(100);
        emit progressChanged(pool.processedCount(), pool.totalTasks(), {});
    }
    pool.waitForAll();

    m_taskFailedFiles = pool.failedFiles();
    plan.failedFiles.append(m_taskFailedFiles);
    if (m_taskCancelled.loadRelaxed()) return false;

    // ── 删除分支：注释禁用（删除同步暂停，本次不处理 filesToDelete）──

    // 下载成功者计入 m_downloadedFiles（全量上传场景为空）
    QSet<QString> failedSet;
    for (const auto& f : plan.failedFiles) { int p = f.indexOf(" ("); failedSet.insert(p > 0 ? f.left(p) : f); }
    for (const auto& op : plan.filesToDownload) {
        if (failedSet.contains(op.canonicalPath)) continue;
        QString rel = op.canonicalPath; if (rel.startsWith("music/")) rel = rel.mid(6);
        QString localPath = PlatformPathHelper::downloadDir() + "/" + rel;
#ifdef Q_OS_ANDROID
        if (localPath.toUtf8().size() > MAX_LOCAL_PATH_UTF8) {
            int dirUtf8 = QFileInfo(localPath).absolutePath().toUtf8().size();
            int maxNameUtf8 = MAX_LOCAL_PATH_UTF8 - dirUtf8 - 1;
            QString safeName = sanitizeLongFileName(QFileInfo(localPath).fileName(), op.hash.left(4), maxNameUtf8);
            localPath = QFileInfo(localPath).absolutePath() + "/" + safeName;
        }
#endif
        m_downloadedFiles.append({op.hash, localPath});
    }
    return m_taskFailedFiles.isEmpty();
}


// ═══════════════════ Phase: 数据同步 ═══════════════════

bool SyncWorker::syncData(FtpSession& ftpSession)
{
    QByteArray remoteJson;
    if (m_isIncremental.value_or(false)) {
        remoteJson = ftpSession.downloadBytes(ftpSession.makeRemotePath("sync_data.json"));
    }

    QByteArray merged = m_coordinator
        ? m_coordinator->merge(m_localSyncDataJson, remoteJson)
        : m_localSyncDataJson;
    m_mergedSyncData = merged;

    ftpSession.uploadBytes(ftpSession.makeRemotePath("sync_data.json"), merged);

    return true;
}

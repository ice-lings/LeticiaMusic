#include "ftp_session.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QThread>

#include <cstdio>
#include <cstring>

#include "../utils/logger.h"

// ── URL 构建（处理中文/日文路径编码） ──────────────────────────────
QString FtpSession::makeFtpUrl(const QString& path) const
{
    QString cleanPath = path;
    while (cleanPath.startsWith('/')) cleanPath = cleanPath.mid(1);

    QUrl url;
    url.setScheme("ftp");
    url.setHost(m_config.host);
    if (m_config.port != 21) url.setPort(m_config.port);
    url.setPath("/" + cleanPath);

    return url.toString(QUrl::FullyEncoded);
}

// ── 句柄生命周期 ────────────────────────────────────────────────
bool FtpSession::createHandle()
{
    if (m_handle) return true;
    m_handle = curl_easy_init();
    if (!m_handle) {
        m_lastError = "curl_easy_init 失败";
        Log.debug("[FtpSession] curl_easy_init 失败");
        return false;
    }
    setCommonOptions();
    Log.debug(QString("[FtpSession] 句柄已创建 handle=%1").arg((quintptr)m_handle, 0, 16));
    return true;
}

bool FtpSession::resetHandle()
{
    if (!m_handle) {
        return createHandle();
    }
    curl_easy_reset(m_handle);
    setCommonOptions();
    return true;
}

void FtpSession::destroyHandle()
{
    if (m_handle) {
        curl_easy_cleanup(m_handle);
        m_handle = nullptr;
        Log.debug("[FtpSession] 句柄已释放");
    }
}

void FtpSession::initGlobal()
{
    static bool ok = []{ curl_global_init(CURL_GLOBAL_DEFAULT); return true; }();
    (void)ok;
}

void FtpSession::cleanupGlobal() { curl_global_cleanup(); }

void FtpSession::setCommonOptions()
{
    curl_easy_setopt(m_handle, CURLOPT_USERNAME, m_config.user.toUtf8().constData());
    curl_easy_setopt(m_handle, CURLOPT_PASSWORD, m_config.password.toUtf8().constData());
    curl_easy_setopt(m_handle, CURLOPT_CONNECTTIMEOUT, (long)m_config.timeoutSec);
    curl_easy_setopt(m_handle, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(m_handle, CURLOPT_LOW_SPEED_LIMIT, 1024L);
    curl_easy_setopt(m_handle, CURLOPT_LOW_SPEED_TIME, 20L);
    curl_easy_setopt(m_handle, CURLOPT_FTP_FILEMETHOD, (long)CURLFTPMETHOD_NOCWD);
    curl_easy_setopt(m_handle, CURLOPT_ERRORBUFFER, m_errorBuffer);
    m_errorBuffer[0] = '\0';
    curl_easy_setopt(m_handle, CURLOPT_NOSIGNAL, 1L);
#ifndef Q_OS_ANDROID
    curl_easy_setopt(m_handle, CURLOPT_FTPPORT, "-");
    curl_easy_setopt(m_handle, CURLOPT_FTP_USE_EPSV, 0L);
    curl_easy_setopt(m_handle, CURLOPT_FTP_USE_EPRT, 0L);
    curl_easy_setopt(m_handle, CURLOPT_FTP_SKIP_PASV_IP, 1L);
#endif
}

// ── 静态回调 ────────────────────────────────────────────────────
size_t FtpSession::readFromFile(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    QFile* file = static_cast<QFile*>(userdata);
    size_t total = size * nmemb;
    qint64 bytes = file->read(static_cast<char*>(ptr), total);
    return bytes > 0 ? static_cast<size_t>(bytes) : 0;
}

size_t FtpSession::writeToFile(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    QFile* file = static_cast<QFile*>(userdata);
    size_t total = size * nmemb;
    qint64 written = file->write(static_cast<const char*>(ptr), total);
    return written < 0 ? 0 : static_cast<size_t>(written);
}

// ── 远端文件大小（SIZE 命令 + CONTENT_LENGTH_DOWNLOAD_T） ──────────
qint64 FtpSession::fileSize(const QString& remotePath)
{
    if (!resetHandle()) return -1;
    QByteArray urlBytes = makeFtpUrl(remotePath).toUtf8();
    curl_easy_setopt(m_handle, CURLOPT_URL, urlBytes.constData());
    curl_easy_setopt(m_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(m_handle, CURLOPT_TIMEOUT, 10L);

    struct curl_slist* cmds = nullptr;
    cmds = curl_slist_append(cmds, QString("SIZE %1").arg(remotePath).toUtf8().constData());
    curl_easy_setopt(m_handle, CURLOPT_QUOTE, cmds);

#if defined(Q_OS_WIN)
    FILE* nullFile = fopen("NUL", "w");
#else
    FILE* nullFile = fopen("/dev/null", "w");
#endif
    curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, nullFile);   // 抑制 stdout 垃圾

    CURLcode res = curl_easy_perform(m_handle);
    curl_slist_free_all(cmds);

    curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, stdout);
    if (nullFile) fclose(nullFile);

    if (res != CURLE_OK) return -1;

    curl_off_t filesize = -1;
    CURLcode info = curl_easy_getinfo(m_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &filesize);
    return (info == CURLE_OK && filesize >= 0) ? static_cast<qint64>(filesize) : -1;
}

// ── 递归 MKD ────────────────────────────────────────────────────
int FtpSession::ensureRemoteDir(const QString& remoteDir)
{
    if (!resetHandle()) {
        m_lastError = "句柄重置失败";
        return 0;
    }
    QByteArray rootUrlBytes = makeFtpUrl("").toUtf8();

    QStringList parts = remoteDir.split('/', Qt::SkipEmptyParts);
    QString mkdPath = "/";     // MKD 用：/MUSIC/...
    QString cacheKey;          // 缓存键：MUSIC/...（与 insert 形态一致）
    int okCount = 0;

    for (const QString& part : parts) {
        mkdPath += part + "/";
        if (!cacheKey.isEmpty()) cacheKey += '/';
        cacheKey += part;

        // 已知存在 → 跳过 MKD
        if (m_cache && m_cache->contains(cacheKey)) {
            ++okCount;
            continue;
        }

        curl_easy_setopt(m_handle, CURLOPT_URL, rootUrlBytes.constData());
        curl_easy_setopt(m_handle, CURLOPT_NOBODY, 1L);

        struct curl_slist* commands = nullptr;
        QString cmd = "MKD " + mkdPath;
        commands = curl_slist_append(commands, cmd.toUtf8().constData());
        curl_easy_setopt(m_handle, CURLOPT_QUOTE, commands);

        CURLcode res = curl_easy_perform(m_handle);
        curl_slist_free_all(commands);

        // CURLE_OK=真新建（空 NAS）、CURLE_QUOTE_ERROR(21)=已存在 → 两者都视为"存在"并缓存
        if (res == CURLE_OK || res == CURLE_QUOTE_ERROR) {
            ++okCount;
            if (m_cache) m_cache->insert(cacheKey);
        } else {
            Log.warning(QString("[FtpSession] MKD 失败 %1 → CURLcode=%2 (%3)")
                .arg(mkdPath).arg(static_cast<int>(res)).arg(curl_easy_strerror(res)));
        }
    }

    return okCount;
}

// ── 流式上传（预检跳过 + 建目录 + 上传 + 后验证） ────────────────────
FtpSession::TransferResult FtpSession::uploadFile(const QString& localPath, const QString& remotePath)
{
    QFileInfo fi(localPath);
    if (!fi.exists() || !fi.isFile()) {
        m_lastError = "本地文件不存在";
        Log.error(QString("[FtpSession] 本地文件不存在: %1").arg(localPath));
        return TransferResult::Failed;
    }
    qint64 localSize = fi.size();

    // 预检：远端已存在且大小一致 → 跳过
    if (fileSize(remotePath) == localSize) {
        Log.info(QString("[FtpSession] 远端已存在，跳过: %1 (%2 B)").arg(remotePath).arg(localSize));
        return TransferResult::Skipped;
    }

    // 确保远端目录（可空缓存）
    QString remoteDir = QFileInfo(remotePath).path();
    if (remoteDir == QLatin1String(".")) remoteDir.clear();
    if (!remoteDir.isEmpty()) {
        resetHandle();
        int partsCount = remoteDir.count('/') + 1;
        if (ensureRemoteDir(remoteDir) < partsCount) {
            m_lastError = QString("远端目录创建失败: %1").arg(remoteDir);
            Log.error(QString("[FtpSession] 上传前目录检查失败: %1").arg(remoteDir));
            return TransferResult::Failed;
        }
    }

    // 上传准备：无条件 resetHandle，清掉预检/MKD 残留
    if (!resetHandle()) {
        m_lastError = "句柄重置失败";
        return TransferResult::Failed;
    }

    QByteArray urlBytes = makeFtpUrl(remotePath).toUtf8();
    curl_easy_setopt(m_handle, CURLOPT_URL, urlBytes.constData());
    curl_easy_setopt(m_handle, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(m_handle, CURLOPT_READFUNCTION, readFromFile);

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "无法打开本地文件";
        Log.error(QString("[FtpSession] 无法打开本地文件: %1").arg(localPath));
        return TransferResult::Failed;
    }
    curl_easy_setopt(m_handle, CURLOPT_READDATA, &file);
    curl_easy_setopt(m_handle, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(localSize));

    CURLcode res = curl_easy_perform(m_handle);
    file.close();
    if (file.error() != QFileDevice::NoError) {
        Log.warning(QString("[FtpSession] 上传后close失败: %1 error=%2 (%3)")
            .arg(localPath).arg((int)file.error()).arg(file.errorString()));
    }

    if (res != CURLE_OK) {
        m_lastError = QString("上传失败: %1").arg(curl_easy_strerror(res));
        Log.error(QString("[FtpSession] 上传失败: %1 (%2)").arg(remotePath).arg(curl_easy_strerror(res)));
        return TransferResult::Failed;
    }

    // 后验证：远端 size == 本地 size
    qint64 remoteSize = fileSize(remotePath);
    if (remoteSize != localSize) {
        m_lastError = QString("上传后size=%1, 预期=%2").arg(remoteSize).arg(localSize);
        Log.error(QString("[FtpSession] 上传后大小不匹配: %1 local=%2 remote=%3")
            .arg(remotePath).arg(localSize).arg(remoteSize));
        return TransferResult::Failed;
    }

    m_lastError.clear();
    Log.info(QString("[FtpSession] 上传成功: %1 -> %2").arg(localPath).arg(remotePath));
    return TransferResult::Success;
}

// ── 流式下载（远端 size 预检 + 流式 RETR 写盘 + 后验证） ──────────────
FtpSession::TransferResult FtpSession::downloadFile(const QString& remotePath, const QString& localPath)
{
    qint64 remoteSize = fileSize(remotePath);
    if (remoteSize < 0) {
        m_lastError = "远端文件不存在";
        Log.error(QString("[FtpSession] 远端文件不存在或查询失败: %1").arg(remotePath));
        return TransferResult::Failed;
    }

    // 预检：本地已存在且大小一致 → 跳过
    QFileInfo fi(localPath);
    if (fi.exists() && fi.isFile() && fi.size() == remoteSize) {
        Log.info(QString("[FtpSession] 本地已存在，跳过: %1 (%2 B)").arg(localPath).arg(remoteSize));
        return TransferResult::Skipped;
    }

    // 创建本地父目录
    QDir().mkpath(fi.absolutePath());

    // 下载准备：清掉 fileSize 残留的 NOBODY/QUOTE
    if (!resetHandle()) {
        m_lastError = "句柄重置失败";
        return TransferResult::Failed;
    }

    QByteArray urlBytes = makeFtpUrl(remotePath).toUtf8();
    curl_easy_setopt(m_handle, CURLOPT_URL, urlBytes.constData());
    curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, writeToFile);

    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastError = "无法打开本地文件写入";
        Log.error(QString("[FtpSession] 无法打开本地文件写入: %1").arg(localPath));
        return TransferResult::Failed;
    }
    curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &file);

    CURLcode res = curl_easy_perform(m_handle);
    file.close();
    if (file.error() != QFileDevice::NoError) {
        Log.warning(QString("[FtpSession] 下载close失败: %1 error=%2 (%3)")
            .arg(localPath).arg((int)file.error()).arg(file.errorString()));
    }

    if (res != CURLE_OK) {
        m_lastError = QString("下载失败: %1").arg(curl_easy_strerror(res));
        Log.error(QString("[FtpSession] 下载失败: %1 → CURLcode=%2 (%3) err=%4")
            .arg(remotePath).arg((int)res).arg(curl_easy_strerror(res)).arg(m_errorBuffer));
        return TransferResult::Failed;
    }

    // 后验证：本地实际 size == 远端 size
    qint64 localSize = QFileInfo(localPath).size();
    if (localSize != remoteSize) {
        m_lastError = QString("下载后本地size=%1, 预期=%2").arg(localSize).arg(remoteSize);
        Log.error(QString("[FtpSession] 下载后大小不匹配: %1 local=%2 remote=%3")
            .arg(localPath).arg(localSize).arg(remoteSize));
        return TransferResult::Failed;
    }

    m_lastError.clear();
    Log.info(QString("[FtpSession] 下载成功: %1 -> %2 (%3 B)").arg(remotePath).arg(localPath).arg(remoteSize));
    return TransferResult::Success;
}

QString FtpSession::makeRemotePath(const QString& relativePath) const
{
    QString root = m_config.syncRoot;
    while (root.endsWith('/')) root.chop(1);
    QString rel = relativePath;
    while (rel.startsWith('/')) rel = rel.mid(1);
    return root.isEmpty() ? rel : root + "/" + rel;
}

size_t FtpSession::writeToString(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    QByteArray* out = static_cast<QByteArray*>(userdata);
    size_t total = size * nmemb;
    out->append(static_cast<const char*>(ptr), static_cast<int>(total));
    return total;
}

size_t FtpSession::readFromBuffer(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    UploadBuffer* buf = static_cast<UploadBuffer*>(userdata);
    size_t maxCopy = size * nmemb;
    int remaining = buf->data->size() - buf->pos;
    if (remaining <= 0) return 0;
    size_t toCopy = qMin<size_t>(maxCopy, static_cast<size_t>(remaining));
    memcpy(ptr, buf->data->constData() + buf->pos, toCopy);
    buf->pos += static_cast<int>(toCopy);
    return toCopy;
}

QByteArray FtpSession::downloadBytes(const QString& remotePath)
{
    if (!resetHandle()) { m_lastError = "句柄重置失败"; return QByteArray(); }

    QByteArray out;
    QByteArray urlBytes = makeFtpUrl(remotePath).toUtf8();
    curl_easy_setopt(m_handle, CURLOPT_URL, urlBytes.constData());
    curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &out);

    CURLcode res = curl_easy_perform(m_handle);
    if (res != CURLE_OK) {
        m_lastError = QString("downloadBytes 失败: %1").arg(curl_easy_strerror(res));
        Log.warning(QString("[FtpSession] downloadBytes 失败: %1 → %2")
            .arg(remotePath).arg(curl_easy_strerror(res)));
        return QByteArray();   // 远端不存在/失败 → 空
    }

    m_lastError.clear();
    Log.info(QString("[FtpSession] downloadBytes 成功: %1 (%2 B)").arg(remotePath).arg(out.size()));
    return out;
}

bool FtpSession::uploadBytes(const QString& remotePath, const QByteArray& data)
{
    QString remoteDir = QFileInfo(remotePath).path();
    if (remoteDir == QLatin1String(".")) remoteDir.clear();
    if (!remoteDir.isEmpty()) {
        resetHandle();
        ensureRemoteDir(remoteDir);
    }

    if (!resetHandle()) { m_lastError = "句柄重置失败"; return false; }

    UploadBuffer buf{ &data, 0 };
    QByteArray urlBytes = makeFtpUrl(remotePath).toUtf8();
    curl_easy_setopt(m_handle, CURLOPT_URL, urlBytes.constData());
    curl_easy_setopt(m_handle, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(m_handle, CURLOPT_READFUNCTION, readFromBuffer);
    curl_easy_setopt(m_handle, CURLOPT_READDATA, &buf);
    curl_easy_setopt(m_handle, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(data.size()));

    CURLcode res = curl_easy_perform(m_handle);
    if (res != CURLE_OK) {
        m_lastError = QString("uploadBytes 失败: %1").arg(curl_easy_strerror(res));
        Log.error(QString("[FtpSession] uploadBytes 失败: %1 → %2")
            .arg(remotePath).arg(curl_easy_strerror(res)));
        return false;
    }

    m_lastError.clear();
    Log.info(QString("[FtpSession] uploadBytes 成功: %1 (%2 B)").arg(remotePath).arg(data.size()));
    return true;
}
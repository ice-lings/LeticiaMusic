#ifndef FTP_SESSION_H
#define FTP_SESSION_H

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QList>

#include <unordered_set>
#include <shared_mutex>
#include <mutex>

#include <curl/curl.h>

// FTP 连接配置
struct FtpConfig {
    QString host;
    int port = 21;
    QString user = "admin";
    QString password = "";
    QString syncRoot = "MUSIC/LeticiaMusic"; // 不含前导 /
    int timeoutSec = 30;
    bool usePassiveMode = true;  // 预留：实际模式由 setCommonOptions 平台分支决定
};

// 目录列表条目
struct FtpEntry {
    QString name;
    qint64 size = 0;
    bool isDir = false;
    QString rawLine;
};

// 远端目录存在性缓存（线程安全，供线程池多 worker 共享）
class DirExistsCache
{
public:
    bool contains(const QString& path)
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_dirs.count(path) > 0;
    }

    void insert(const QString& path)
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_dirs.insert(path);
    }

    void insertWithParents(const QString& path)
    {
        QStringList parts = path.split('/', Qt::SkipEmptyParts);
        QString cumulative;
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        for (const QString& part : parts) {
            if (!cumulative.isEmpty()) cumulative += '/';
            cumulative += part;
            m_dirs.insert(cumulative);
        }
    }

private:
    std::unordered_set<QString> m_dirs;
    mutable std::shared_mutex m_mutex;
};

// 单 CURL 句柄的轻量 FTP 会话（非 QObject，线程私有，RAII）
// 步骤 1 范围：仅传输核心（上传/下载/SIZE/建目录）；元操作延后到步骤 5。
class FtpSession
{
public:
    enum class TransferResult { Success, Skipped, Failed };

    FtpSession() = default;
    explicit FtpSession(const FtpConfig& config) : m_config(config) {}
    ~FtpSession() { destroyHandle(); }

    FtpSession(const FtpSession&) = delete;
    FtpSession& operator=(const FtpSession&) = delete;
    FtpSession(FtpSession&&) = delete;
    FtpSession& operator=(FtpSession&&) = delete;

    static void initGlobal();
    static void cleanupGlobal();

    void setConfig(const FtpConfig& config) { m_config = config; }
    void setCache(DirExistsCache* cache) { m_cache = cache; }

    // 远端文件查询（CURLINFO_CONTENT_LENGTH_DOWNLOAD_T，无 VERBOSE）
    qint64 fileSize(const QString& remotePath);
    bool fileExists(const QString& remotePath) { return fileSize(remotePath) >= 0; }

    // 递归 MKD（配合可空 DirExistsCache*）
    int ensureRemoteDir(const QString& remoteDir);

    // 流式上传 / 下载（含预检跳过 + 后验证）
    TransferResult uploadFile(const QString& localPath, const QString& remotePath);
    TransferResult downloadFile(const QString& remotePath, const QString& localPath);

    QByteArray downloadBytes(const QString& remotePath);
    bool       uploadBytes(const QString& remotePath, const QByteArray& data);
    QString    makeRemotePath(const QString& relativePath) const;

    QString lastError() const { return m_lastError; }

private:
    bool createHandle();
    bool resetHandle();
    void destroyHandle();
    void setCommonOptions();

    QString makeFtpUrl(const QString& path) const;

    static size_t readFromFile(void* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t writeToFile(void* ptr, size_t size, size_t nmemb, void* userdata);

    struct UploadBuffer { const QByteArray* data; int pos; };
    static size_t writeToString(void* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t readFromBuffer(void* ptr, size_t size, size_t nmemb, void* userdata);

    FtpConfig m_config;
    CURL* m_handle = nullptr;
    char m_errorBuffer[CURL_ERROR_SIZE] = {};
    DirExistsCache* m_cache = nullptr;
    QString m_lastError;
};

#endif // FTP_SESSION_H

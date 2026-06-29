#include "musiccovermanager.h"


#include "../../interfaces/idatabaseservice.h"

#include "../../utils/logger.h"

#include <QString>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QImage>
#include <QCoreApplication>
#include <QCryptographicHash>

MusicCoverManager::MusicCoverManager(QObject *parent, IDatabaseService* databaseService)
    : QObject{parent}, m_databaseService(databaseService)
{}

bool MusicCoverManager::initialize()
{
    // 核心1：基础空指针兜底校验，数据库服务未初始化直接失败
    if (m_databaseService == nullptr) {
        Log.error("--Cover-- Database service is null, cover manager initialize failed!");
        return false;
    }

    // 核心2：初始化封面表（建表/表结构升级），表初始化失败则整体初始化失败
    bool tableInited = initializeTable();
    if (!tableInited) {
        Log.error("--Cover-- Cover table initialize failed, cover manager initialize failed!");
        return false;
    }

    // 核心3：清空内存哈希，防止多次初始化导致旧数据残留（关键操作，避免脏数据）
    m_coverHashPathMap.clear();

    // 核心4：判断表是否为空，分场景处理
    bool isTableEmpty = m_databaseService->isTableEmpty(TABLE_NAME);
    if (isTableEmpty) {
        // 场景1：表为空（首次运行/数据库重建），正常流程，仅打警告日志提示
        Log.warning(QString("--Cover-- Table %1 is empty, no cover hash-path map load to memory.").arg(TABLE_NAME));
        return true;
    }

    // 场景2：表非空，构建查询选项+调用select，加载「哈希-路径」到内存
    Log.info(QString("--Cover-- Table %1 has data, start loading cover hash-path map to memory...").arg(TABLE_NAME));

    // 核心：利用QueryOptions链式set方法构建查询条件，仅查需要的2个字段（轻量化查询）
    // 仅设置columns，不设置where/order/limit等，实现「全表查询指定字段」，贴合你的封装设计
    QueryOptions queryOpt;
    queryOpt.setColumns(QStringList() << "content_hash" << "file_path");

    // 调用你封装的select函数，执行查询（返回QVector<QVariantMap>）
    QVector<QVariantMap> coverMapVec = m_databaseService->select(TABLE_NAME, queryOpt);

    // 核心5：校验查询结果，无数据则打警告并返回（兼容查询异常/结果为空）
    if (coverMapVec.isEmpty()) {
        Log.warning(QString("--Cover-- Query table %1 return empty, no cover map load to memory.").arg(TABLE_NAME));
        return true;
    }

    // 核心6：遍历查询结果，过滤脏数据，加载有效映射到内存哈希
    int loadSuccessCount = 0; // 统计成功加载的有效记录数
    for (const QVariantMap &item : coverMapVec) {
        // 严格校验：content_hash和file_path均非空才加载（过滤数据库脏数据）
        QString contentHash = item.value("content_hash").toString().trimmed();
        QString filePath = item.value("file_path").toString().trimmed();
        if (contentHash.isEmpty() || filePath.isEmpty()) {
            Log.warning("--Cover-- Skip invalid cover data: content_hash or file_path is empty.");
            continue;
        }
        // 加载到内存哈希（键：封面哈希，值：本地文件路径）
        m_coverHashPathMap[contentHash] = filePath;
        loadSuccessCount++;
    }

    // 核心7：加载完成，打印统计日志（区分成功数/总查询数，方便调试排障）
    Log.info(QString("--Cover-- Cover hash-path map load completed! Success load %1 valid records (total query %2 records) to memory.")
                 .arg(loadSuccessCount).arg(coverMapVec.count()));

    // 所有流程执行完成，初始化成功
    return true;
}


void MusicCoverManager::TrySaveCover(const QImage &cover)
{
    CoverInfo info;
    info.contentHash = this->calculateImageHash16(cover);
    info.filePath = this->getCoverSaveAbsolutePath(info.contentHash);

    // 基础校验：哈希/路径为空，直接返回
    if (info.contentHash.isEmpty() || info.filePath.isEmpty()) {
        Log.warning("--Cover-- Skip save cover: hash or file path is empty!");
        return;
    }

    // ======================================
    // 新增核心：双重存在性检查，避免重复保存/插入
    // ======================================
    bool isDuplicate = false;
    // 1. 先查内存哈希（最快，优先拦截重复）
    if (m_coverHashPathMap.contains(info.contentHash)) {
        isDuplicate = true;
        Log.info(QString("--Cover-- Skip save cover: hash exists in memory cache! Hash: %1").arg(info.contentHash));
    }
    // TODO:
    // 2. 内存无则查数据库（容错兜底，防止内存数据丢失）
    // else if (m_databaseService != nullptr) {
    //     isDuplicate = m_databaseService->exists(TABLE_NAME, "content_hash", info.contentHash);
    //     if (isDuplicate) {
    //         Log.info(QString("--Cover-- Skip save cover: hash exists in database! Hash: %s").arg(info.contentHash));
    //         // 可选：数据库有但内存无，将哈希-路径同步到内存（保证内存与数据库一致）
    //         // 需补充「通过哈希查路径」的接口，下方有说明
    //         // syncHashPathToMemory(info.contentHash);
    //     }
    // }

    // 检查到重复，直接返回，跳过后续所有操作
    if (isDuplicate) {
        return;
    }

    // 原逻辑：检查通过（无重复），才执行文件保存
    bool saveSuccess = cover.save(info.filePath, "JPG", 85);

    if (saveSuccess) {
        // 补全 CoverInfo 字段（文件保存后才可靠）
        info.fileValid = true;
        info.fileSize = QFile(info.filePath).size();
        info.lastAccessed = 0;
        info.createdAt = QDateTime::currentDateTime();

        // 先尝试插入数据库，失败则回滚删除文件
        if (this->insertCoverInfoToDb(info)) {
            m_coverHashPathMap[info.contentHash] = info.filePath;
            Log.info(QString("--Cover-- Save cover success! Path: %1").arg(info.filePath));
        } else {
            QFile::remove(info.filePath);
            Log.error(QString("--Cover-- DB 插入失败，已回滚删除: %1").arg(info.filePath));
        }
    } else {
        Log.warning(QString("--Cover-- Save cover failed! Path: %1").arg(info.filePath));
    }

}

QString MusicCoverManager::saveCoverSafe(const QImage& cover)
{
    if (cover.isNull()) return "";

    const QString hash = calculateImageHash16(cover);
    if (hash.isEmpty()) return "";

    const QString savePath = getCoverSaveAbsolutePath(hash);
    if (savePath.isEmpty()) return "";

    QMutexLocker locker(&coverWriteMutex);

    if (m_coverHashPathMap.contains(hash)) {
        return hash;
    }

    bool saved = cover.save(savePath, "JPG", 85);
    if (!saved) {
        return "";
    }

    m_coverHashPathMap[hash] = savePath;

    return hash;
}

QString MusicCoverManager::registerCoverIfExists(const QString& contentHash)
{
    if (contentHash.isEmpty()) return "";
    if (m_coverHashPathMap.contains(contentHash))
        return m_coverHashPathMap[contentHash];

    QString savePath = getCoverSaveAbsolutePath(contentHash);
    if (QFile::exists(savePath)) {
        m_coverHashPathMap[contentHash] = savePath;
        persistCoverToDb(contentHash);
        return savePath;
    }
    return "";
}

void MusicCoverManager::persistCoverToDb(const QString& hash)
{
    if (hash.isEmpty() || !m_coverHashPathMap.contains(hash)) return;

    const QString filePath = m_coverHashPathMap[hash];

    CoverInfo info;
    info.contentHash = hash;
    info.filePath = filePath;
    info.fileValid = true;
    info.fileSize = QFileInfo(filePath).size();
    info.lastAccessed = 0;
    info.createdAt = QDateTime::currentDateTime();
    insertCoverInfoToDb(info);
}

QString MusicCoverManager::getCoverPath(const QString &contentHash)
{
    if (!m_coverHashPathMap.contains(contentHash)) {
        return "";
    }
    QString filePath = m_coverHashPathMap[contentHash];


    if (!QFile::exists(filePath)) {
        m_coverHashPathMap.remove(contentHash);
        return "";
    }

    return filePath;
}

bool MusicCoverManager::initializeTable()
{
    const QStringList coversSchema = {
        "content_hash CHAR(32) PRIMARY KEY",  // 核心：32位MD5哈希唯一标识
        "file_path TEXT NOT NULL UNIQUE",     // 核心：封面本地绝对路径
        "file_size INTEGER NOT NULL",         // 核心：封面文件大小（字节），用于完整性校验
        "file_valid TINYINT NOT NULL DEFAULT 1", // 新增刚需：文件有效性标记 1=有效 0=无效
        "last_accessed INTEGER DEFAULT 0",      // 核心：最后访问秒级时间戳，缓存清理依据
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP" // 基础：创建时间
    };

    if (!m_databaseService->tableExists(TABLE_NAME)) {
        if (!m_databaseService->createTable(TABLE_NAME, coversSchema)) {
            Log.error("Failed to create --Cover-- table: " + m_databaseService->lastError());
            return false;
        }
        Log.info("--Cover-- table created successfully");
        return true;
    } else {
        Log.debug("--Cover-- table already exists");
        return true;
    }

    return true;
}

QString MusicCoverManager::calculateImageHash(const QImage &image, int hashSize)
{

    if (image.isNull()) {
        return "";
    }

    // 边界2：缩放尺寸最小限制4x4，避免无效尺寸
    hashSize = qMax(4, hashSize);

    // 步骤1：快速缩放到固定小尺寸（放弃插值画质，优先速度，封面展示无感知）
    QImage smallImage = image.scaled(hashSize, hashSize,
                                     Qt::KeepAspectRatio,    // 保持宽高比，避免拉伸失真
                                     Qt::FastTransformation); // 快速缩放模式，核心提速点

    // 步骤2：创建8位灰度图缓冲区（仅单通道，数据量最小，计算最快）
    QImage grayImage(hashSize, hashSize, QImage::Format_Grayscale8);
    // 遍历像素计算灰度值
    for (int y = 0; y < smallImage.height() && y < hashSize; ++y) {
        for (int x = 0; x < smallImage.width() && x < hashSize; ++x) {
            QColor pixelColor = smallImage.pixelColor(x, y);
            // 边界防护：避免无效像素色值
            if (!pixelColor.isValid()) {
                grayImage.setPixel(x, y, qRgb(0, 0, 0));
                continue;
            }
            // ITU-R BT.601 标准亮度公式（Qt官方推荐，灰度计算最准确）
            int red = pixelColor.red();
            int green = pixelColor.green();
            int blue = pixelColor.blue();
            uchar grayValue = qRound(0.299 * red + 0.587 * green + 0.114 * blue);
            // 写入灰度图像素（单通道，qRgb三个值相同即可）
            grayImage.setPixel(x, y, qRgb(grayValue, grayValue, grayValue));
        }
    }

    // 步骤3：直接读取像素二进制缓冲区（最快的数转方式，无额外拷贝）
    QByteArray imageData((const char*)grayImage.bits(), grayImage.sizeInBytes());

    // 步骤4：Qt原生MD5哈希计算（和你的封面Hash存储方案统一，32位小写）
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(imageData);
    return hash.result().toHex().toLower();
}

QString MusicCoverManager::calculateImageHash16(const QImage &image)
{
    return calculateImageHash(image, 16);
}

QString MusicCoverManager::getCoverSaveAbsolutePath(const QString &contentHash, const QString &rootSubDir)
{
    // 边界1：校验哈希有效性（必须32位，和calculateImageHash16生成的一致）
    if (contentHash.isEmpty() || contentHash.length() != 32) {
        return "";
    }
    
    // 安全验证：防止路径遍历攻击
    if (rootSubDir.contains("..") || rootSubDir.contains("//") || rootSubDir.startsWith('/')) {
        Log.error("[MusicCoverManager] Invalid rootSubDir path: {}", rootSubDir.toStdString());
        return "";
    }
    
    if (contentHash.contains("..") || contentHash.contains("/") || contentHash.contains("\\")) {
        Log.error("[MusicCoverManager] Invalid contentHash: {}", contentHash.toStdString());
        return "";
    }

    // 步骤1：获取封面根目录
#ifdef Q_OS_ANDROID
    QString exeDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString exeDir = QCoreApplication::applicationDirPath();
#endif
    // 步骤2：生成封面根目录（exeDir/rootSubDir/，如：/data/data/.../covers/）
    QString coverRootDir = QDir(exeDir).filePath(rootSubDir);

    // 步骤3：按Hash拆分分目录（前2位=一级目录，3-4位=二级目录，沿用原有规则）
    QString dirLevel1 = contentHash.left(2);  // 取哈希前2位
    QString dirLevel2 = contentHash.mid(2, 2); // 取哈希第3-4位
    // 步骤4：拼接完整的保存目录（exe/covers/一级/二级/）
    QString saveDir = QDir(coverRootDir).filePath(QString("%1/%2").arg(dirLevel1, dirLevel2));

    // 步骤5：确保保存目录存在（递归创建，不存在则创建，存在则无操作，跨平台）
    QDir dir;
    if (!dir.mkpath(saveDir)) { // 创建失败（如权限不足），返回空
        return "";
    }

    // 步骤6：拼接封面文件绝对路径（保存目录+哈希.jpg，统一JPG格式）
    QString coverFileName = contentHash + ".jpg";
    QString absolutePath = QDir(saveDir).filePath(coverFileName);

    return absolutePath;
}

bool MusicCoverManager::deleteCover(const QString& coverHash)
{
    if (coverHash.isEmpty()) return false;

    QString filePath = m_coverHashPathMap.value(coverHash);

    // 删除物理文件
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        if (QFile::remove(filePath)) {
            Log.info(QString("--Cover-- Deleted cover file: %1").arg(filePath));
        } else {
            Log.warning(QString("--Cover-- Failed to delete cover file: %1").arg(filePath));
        }
    }

    // 从内存缓存移除
    m_coverHashPathMap.remove(coverHash);

    // 从数据库删除
    if (m_databaseService) {
        QVariantMap whereParams;
        whereParams["content_hash"] = coverHash;
        bool dbResult = m_databaseService->remove(TABLE_NAME, "content_hash = :content_hash", whereParams);
        if (dbResult) {
            Log.info(QString("--Cover-- Removed cover record from DB: %1").arg(coverHash));
        } else {
            Log.warning(QString("--Cover-- Failed to remove cover record from DB: %1").arg(coverHash));
        }
        return dbResult;
    }

    return true;
}

bool MusicCoverManager::insertCoverInfoToDb(const CoverInfo &info)
{
    if (m_databaseService == nullptr) {
        Log.warning("数据库服务未初始化，无法插入封面信息");
        return false;
    }

    // 校验2：核心字段有效性（content_hash和file_path是数据库NOT NULL字段，不能为空）
    if (info.contentHash.isEmpty() || info.filePath.isEmpty()) {
        Log.warning("封面核心字段为空（contentHash/filePath），无法插入");
        return false;
    }

    // 核心：将CoverInfo转换为QVariantMap，严格匹配数据库表cover_storage的字段名
    QVariantMap dataMap;
    dataMap["content_hash"] = info.contentHash;    // 字符串→CHAR(32)
    dataMap["file_path"]    = info.filePath;       // 字符串→TEXT
    dataMap["file_size"]    = info.fileSize;       // qint64→INTEGER
    dataMap["file_valid"]   = info.fileValid ? 1 : 0; // bool→TINYINT（1=有效，0=无效）
    dataMap["last_accessed"]= info.lastAccessed;   // qint64→INTEGER（时间戳）
    // QDateTime→DATETIME：转换为数据库可解析的24小时制字符串（yyyy-MM-dd HH:mm:ss）
    dataMap["created_at"]   = info.createdAt.toString("yyyy-MM-dd HH:mm:ss");

    // 调用数据库服务的insert接口，插入到cover_storage表
    int insertResult = m_databaseService->insertIgnore(TABLE_NAME, dataMap);

    if (insertResult >= 0) {
        Log.debug(QString("封面信息插入数据库成功，哈希：%1").arg(info.contentHash));
        return true;
    } else if (insertResult == -2) {
        Log.debug(QString("封面信息已存在，跳过，哈希：%1").arg(info.contentHash));
        return true;
    } else {
        Log.warning(QString("封面信息插入数据库失败，哈希：%1").arg(info.contentHash));
        return false;
    }
}

QImage MusicCoverManager::prepareCoverImage(const QImage& source, int maxSize, int minSize)
{
    if (source.isNull()) return {};

    int maxDim = qMax(source.width(), source.height());

    if (maxDim > maxSize)
        return source.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (maxDim < minSize)
        return source.scaled(minSize, minSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return source;
}
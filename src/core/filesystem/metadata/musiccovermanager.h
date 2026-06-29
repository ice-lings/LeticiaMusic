#ifndef MUSICCOVERMANAGER_H
#define MUSICCOVERMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QMutex>

class IMusicMetadataExtract;
class IDatabaseService;

class MusicCoverManager : public QObject
{
    Q_OBJECT
public:
    explicit MusicCoverManager(QObject *parent = nullptr,
                               IDatabaseService* databaseService = nullptr);


    bool initialize();

    void TrySaveCover(const QImage& cover);

    QString getCoverPath(const QString &contentHash);


/**
 * @brief 便捷重载函数：固定16x16缩放（兼顾速度+唯一性，封面去重推荐使用）
 * @param image 待计算的QImage封面
 * @return 32位小写MD5哈希字符串
 */
    QString calculateImageHash16(const QImage &image);

    QString saveCoverSafe(const QImage& cover);

    QString registerCoverIfExists(const QString& contentHash);

    void persistCoverToDb(const QString& hash);

    /**
     * @brief 删除封面文件及数据库记录
     * @param coverHash 封面哈希
     * @return true 表示成功删除（或文件本就不存在），false 表示删除失败
     */
    bool deleteCover(const QString& coverHash);

    static QImage prepareCoverImage(const QImage& source, int maxSize = 600, int minSize = 200);

signals:

private:

    struct CoverInfo
    {
        Q_GADGET // 关键：支持Qt元对象系统，可用于QML/模型绑定、QVariant传递
    public:
        // 数据库字段一一对应，类型适配Qt最佳实践
        QString contentHash;   // 对应content_hash：32位MD5封面哈希（主键）
        QString filePath;      // 对应file_path：封面本地绝对路径
        qint64 fileSize;       // 对应file_size：文件大小（字节），qint64兼容大文件，跨平台安全
        bool fileValid;        // 对应file_valid：文件有效性 1=true(有效) 0=false(无效)
        qint64 lastAccessed;   // 对应last_accessed：最后访问秒级时间戳（0=未访问）
        QDateTime createdAt;   // 对应created_at：创建时间，Qt原生类型方便时间处理

        // 便捷构造函数：初始化默认值（匹配数据库默认值）
        CoverInfo() : fileSize(0), fileValid(true), lastAccessed(0) {}
    };


    bool initializeTable();

    const QString TABLE_NAME = "cover_storage";

    /**
 * @brief 快速计算QImage的MD5哈希值（封面去重用，高性能）
 * @param image 待计算的原始QImage（封面图）
 * @param hashSize 预处理缩放尺寸（默认8x8，速度最快；16x16唯一性更强，可按需调整）
 * @return 32位小写MD5哈希字符串，空Image返回空字符串
 * @note 预处理：缩放为固定尺寸→转灰度图，保证相同内容封面哈希一致，且计算极快
 */
    QString calculateImageHash(const QImage &image, int hashSize = 8);

    /**
 * @brief 由封面32位MD5哈希生成保存绝对路径，并确保所有目录存在
 * @param contentHash 由calculateImageHash16生成的32位小写MD5哈希
 * @param rootSubDir  exe同级的根子文件夹名（默认covers，可自定义如"cover_cache"）
 * @return 封面文件的绝对路径（如：D:/exe/covers/e8/a9/e8a9f7c6d5b4a39281706f5e4d3c2b1a.jpg），失败返回空
 * @note 1. 哈希必须是32位小写，否则返回空 2. 统一保存为JPG格式 3. 递归创建目录，跨平台兼容
 */
    QString getCoverSaveAbsolutePath(const QString &contentHash, const QString &rootSubDir = "covers");


    /**
     * @brief 将封面信息插入到cover_storage表
     * @param info 待插入的封面信息结构体（已完成赋值的CoverInfo）
     * @return bool 插入成功返回true，失败返回false
     * @note 1. 自动完成结构体→QVariantMap的类型转换 2. 校验数据库服务指针和核心字段有效性
     */
    bool insertCoverInfoToDb(const CoverInfo& info);

    QHash<QString, QString> m_coverHashPathMap;
    mutable QMutex coverWriteMutex;

    IDatabaseService* m_databaseService = nullptr;
};

#endif // MUSICCOVERMANAGER_H

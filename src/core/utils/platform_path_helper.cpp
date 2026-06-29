#include "platform_path_helper.h"
#include "logger.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef Q_OS_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#endif

QString PlatformPathHelper::appDataDir()
{
#ifdef Q_OS_ANDROID
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    return QCoreApplication::applicationDirPath();
#endif
}

QString PlatformPathHelper::cacheDir()
{
#ifdef Q_OS_ANDROID
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    return appDataDir() + "/cache";
#endif
}

QString PlatformPathHelper::documentDir()
{
#ifdef Q_OS_ANDROID
    // Android 11+ Scoped Storage: DocumentsLocation 不可直接写入
    // 日志等文件使用 appDataDir，通过应用内"导出日志"功能分享给用户
    QString dir = appDataDir();
#else
    QString dir = appDataDir();
#endif
    ensureDir(dir);
    return dir;
}

QString PlatformPathHelper::logDir()
{
#ifdef Q_OS_ANDROID
    // 内部存储保证 100% 可写，无需 Android 权限
    // 日志查看主通道: QtCreator logcat / adb logcat -s Leticia
    // 日志文件路径: /data/data/org.qtproject.example.LeticiaMusic/files/logs
    QString path = appDataDir() + "/logs";
#else
    QString path = appDataDir() + "/logs";
#endif
    ensureDir(path);
    return path;
}

QString PlatformPathHelper::downloadDir()
{
#ifdef Q_OS_ANDROID
    // Android: 使用外部私有目录，USB 可访问，无需特殊权限
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (context.isValid()) {
        QJniObject file = context.callObjectMethod("getExternalFilesDir",
            "(Ljava/lang/String;)Ljava/io/File;", nullptr);
        if (file.isValid()) {
            QJniObject path = file.callObjectMethod("getAbsolutePath",
                "()Ljava/lang/String;");
            QString dir = path.toString();
            if (!dir.isEmpty()) {
                dir += "/Music";
                ensureDir(dir);
                return dir;
            }
        }
    }
    // 回退：内部存储
    QString dir = appDataDir() + "/Music";
    ensureDir(dir);
    return dir;
#else
    // Windows: 可配置路径
    static QString s_customDownloadPath;
    if (!s_customDownloadPath.isEmpty()) {
        ensureDir(s_customDownloadPath);
        return s_customDownloadPath;
    }
    QString dir = appDataDir() + "/Music";
    ensureDir(dir);
    return dir;
#endif
}

void PlatformPathHelper::setDownloadPath(const QString& path)
{
#ifndef Q_OS_ANDROID
    static QString s_customDownloadPath;
    s_customDownloadPath = path;
#else
    Q_UNUSED(path);
#endif
}

QString PlatformPathHelper::exportDir()
{
#ifdef Q_OS_ANDROID
    // 使用外部应用文件目录（/storage/emulated/0/Android/data/<pkg>/files/export）
    // 无需权限即可写入，用户可通过 PC USB 连接或文件管理器访问
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (context.isValid()) {
        QJniObject file = context.callObjectMethod("getExternalFilesDir",
            "(Ljava/lang/String;)Ljava/io/File;", nullptr);
        if (file.isValid()) {
            QJniObject path = file.callObjectMethod("getAbsolutePath",
                "()Ljava/lang/String;");
            QString dir = path.toString();
            if (!dir.isEmpty()) {
                dir += "/export";
                ensureDir(dir);
                return dir;
            }
        }
    }
    // 回退：内部存储
    QString dir = appDataDir() + "/export";
    ensureDir(dir);
    return dir;
#else
    QString dir = appDataDir() + "/export";
    ensureDir(dir);
    return dir;
#endif
}

void PlatformPathHelper::ensureDir(const QString& path)
{
    QDir dir(path);
    if (!dir.exists()) {
        if (dir.mkpath(".")) {
            Log.info(QString("创建目录: %1").arg(path));
        } else {
            Log.warning(QString("目录创建失败: %1").arg(path));
        }
    }
}

void PlatformPathHelper::migrateFile(const QString& oldPath, const QString& newPath)
{
    if (!QFile::exists(oldPath)) return;
    if (QFile::exists(newPath)) return;

    QFileInfo fi(newPath);
    QDir().mkpath(fi.path());

    if (QFile::copy(oldPath, newPath)) {
        Log.info(QString("迁移文件: %1 -> %2").arg(oldPath, newPath));
    } else {
        Log.warning(QString("迁移文件失败: %1 -> %2").arg(oldPath, newPath));
    }
}

void PlatformPathHelper::migrateDirContents(const QString& oldDir, const QString& newDir)
{
    QDir od(oldDir);
    if (!od.exists()) return;

    ensureDir(newDir);

    const auto entries = od.entryInfoList(QDir::Files);
    for (const auto& fi : entries) {
        QString newPath = newDir + "/" + fi.fileName();
        migrateFile(fi.absoluteFilePath(), newPath);
    }
}

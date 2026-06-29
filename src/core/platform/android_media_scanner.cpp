#include "android_media_scanner.h"
#include <android/log.h>
#include <QJniObject>
#include <QJniEnvironment>
#include <QCoreApplication>
#include <QtCore/private/qandroidextras_p.h>
#include <QDirIterator>
#include <QStandardPaths>
#include <QSet>
#include <QFileInfo>
#include <QDir>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "LeticiaScan", __VA_ARGS__)

// 递归收集目录下所有音频文件
static void collectAudioFiles(const QDir& dir, const QSet<QString>& exts, QStringList& out)
{
    const QFileInfoList entries = dir.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo& fi : entries) {
        if (fi.isDir()) {
            collectAudioFiles(QDir(fi.absoluteFilePath()), exts, out);
        } else if (fi.isFile() && exts.contains(fi.suffix().toLower())) {
            out.append(fi.absoluteFilePath());
        }
    }
}

QStringList AndroidMediaScanner::queryAllAudio()
{
    QStringList result;

    LOGI("=== queryAllAudio 开始 ===");

    // === 方案一：MediaStore 查询 ===
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (context.isValid()) {
        LOGI("Context 有效，调用 MediaStoreHelper...");

        QJniObject pathArray = QJniObject::callStaticObjectMethod(
            "org/qtproject/example/MediaStoreHelper",
            "queryAllAudio",
            "(Landroid/content/Context;)[Ljava/lang/String;",
            context.object());

        if (pathArray.isValid()) {
            QJniEnvironment env;
            const jsize count = env->GetArrayLength(pathArray.object<jarray>());
            LOGI("MediaStore 返回 %d 个文件", static_cast<int>(count));

            result.reserve(static_cast<int>(count));
            for (jsize i = 0; i < count; ++i) {
                jobject element = env->GetObjectArrayElement(
                    pathArray.object<jobjectArray>(), i);
                QJniObject str(element);
                if (str.isValid()) {
                    result.append(str.toString());
                }
                env->DeleteLocalRef(element);
            }
        } else {
            LOGI("MediaStore JNI 失败: pathArray 为空");
        }
    } else {
        LOGI("Context 无效");
    }

    // === 方案二：QDir 递归回退 ===
    if (result.isEmpty()) {
        LOGI("MediaStore 返回空，QDir 递归回退...");

        QStringList searchDirs;
        searchDirs << "/sdcard/Music"
                   << "/storage/emulated/0/Music";

        const QSet<QString> exts = {"mp3", "flac", "wav", "ogg", "m4a", "aac", "wma", "ape"};

        for (const QString& dirPath : searchDirs) {
            QDir dir(dirPath);
            LOGI("  扫描: %s 存在=%d 路径=%s",
                 qPrintable(dirPath), dir.exists(), qPrintable(dir.absolutePath()));

            // 诊断：不用过滤器看全部条目
            QFileInfoList all = dir.entryInfoList(QDir::NoDotAndDotDot);
            LOGI("    entryInfoList(无过滤) = %lld 条", static_cast<long long>(all.size()));
            for (int i = 0; i < qMin(3, all.size()); ++i) {
                LOGI("      [%d] %s (dir=%d file=%d hidden=%d readable=%d)",
                     i, qPrintable(all[i].fileName()),
                     all[i].isDir(), all[i].isFile(),
                     all[i].isHidden(), all[i].isReadable());
            }

            if (dir.exists()) {
                QStringList before;
                collectAudioFiles(dir, exts, before);
                LOGI("    QDir 递归发现 %lld 音频", static_cast<long long>(before.size()));
                result.append(before);
            }
        }

        // 去重
        QSet<QString> seen;
        QStringList deduped;
        for (const QString& path : result) {
            if (!seen.contains(path)) {
                seen.insert(path);
                deduped.append(path);
            }
        }
        result = deduped;
    }

    LOGI("=== 扫描完成: %d 个文件 ===", static_cast<int>(result.size()));
    return result;
}

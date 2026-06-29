#ifndef ANDROID_MEDIA_SCANNER_H
#define ANDROID_MEDIA_SCANNER_H

#include <QStringList>

/* 安卓平台 — 通过 MediaStore 查询系统媒体库中的音频文件 */
namespace AndroidMediaScanner {

/* 返回设备上所有音频文件的绝对路径列表 */
QStringList queryAllAudio();

} // namespace AndroidMediaScanner

#endif // ANDROID_MEDIA_SCANNER_H

#ifndef TAGLIB_UTILS_H
#define TAGLIB_UTILS_H

#include <QString>

#include "fileref.h"

namespace TaglibUtils {


inline static TagLib::FileRef createTaglibFileRef(const QString& qtPath) {
    // 按系统做差异化路径转换，返回Taglib的FileRef对象
#ifdef _WIN32
    // Windows系统：QString(UTF-16) → wchar_t*(UTF-16)，用Taglib宽字符接口
    std::wstring widePath = qtPath.toStdWString();
    return TagLib::FileRef(widePath.c_str());
#else
    // Linux/macOS：QString → UTF-8 const char*，用原有接口
    QByteArray utf8Path = qtPath.toUtf8();
    return TagLib::FileRef(utf8Path.constData());
#endif
}


inline QString tagStringToQString(const TagLib::String& str)
{
    return QString::fromUtf8(str.toCString(true));
}

} // namespace TaglibUtils
#endif // TAGLIB_UTILS_H

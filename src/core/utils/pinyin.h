#ifndef PINYINUTILS_H
#define PINYINUTILS_H

#include <QString>

class PinyinUtils
{
public:
    static QString toPinyin(const QString &text);
    static QString sectionKey(const QString &title);
    static QChar firstLetter(const QString &title);

private:
    static QString lookupPinyin(uint32_t unicode);
};

#endif // PINYINUTILS_H
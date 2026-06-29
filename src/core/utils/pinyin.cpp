#include "pinyin.h"
#include "pinyin_data.inc"

#include <QStringList>
#include <QChar>

static bool isCJK(uint32_t cp)
{
    return (cp >= 0x3400 && cp <= 0x4DBF)   // CJK Extension A
        || (cp >= 0x4E00 && cp <= 0x9FFF)   // CJK Basic
        || (cp >= 0xF900 && cp <= 0xFAFF);  // CJK Compatibility
}

QString PinyinUtils::lookupPinyin(uint32_t unicode)
{
    int left = 0;
    int right = PINYIN_TABLE_SIZE - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (pinyinTable[mid].unicode == unicode) {
            return QString::fromLatin1(pinyinTable[mid].pinyin);
        } else if (pinyinTable[mid].unicode < unicode) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return QString();
}

QString PinyinUtils::toPinyin(const QString &text)
{
    if (text.isEmpty()) return QString();

    QString result;
    result.reserve(text.size() * 6);

    for (int i = 0; i < text.size(); ++i) {
        QChar ch = text.at(i);

        if (isCJK(ch.unicode())) {
            QString py = lookupPinyin(ch.unicode());
            if (!py.isEmpty()) {
                result.append(py);
            } else {
                result.append(ch.toLower());
            }
        } else if (ch.isLetter()) {
            result.append(ch.toLower());
        } else if (ch.isDigit()) {
            result.append(ch);
        } else if (ch == ' ' || ch == '\t') {
            result.append(' ');
        }
    }

    return result;
}

QString PinyinUtils::sectionKey(const QString &title)
{
    if (title.isEmpty()) return QStringLiteral("#");

    for (int i = 0; i < title.size(); ++i) {
        QChar ch = title.at(i);

        // 跳过前导的空白字符
        if (ch.isSpace()) continue;

        if (isCJK(ch.unicode())) {
            QString py = lookupPinyin(ch.unicode());
            if (!py.isEmpty()) {
                QChar first = py.at(0).toUpper();
                if (first >= 'A' && first <= 'Z') {
                    return QString(first);
                }
            }
            return QStringLiteral("#");
        } else if (ch.isLetter()) {
            QChar upper = ch.toUpper();
            if (upper >= 'A' && upper <= 'Z') {
                return QString(upper);
            }
            return QStringLiteral("#");
        } else if (ch.isDigit()) {
            return QStringLiteral("0");
        } else {
            return QStringLiteral("#");
        }
    }

    return QStringLiteral("#");
}

QChar PinyinUtils::firstLetter(const QString &title)
{
    QString key = sectionKey(title);
    if (key.length() == 1) {
        return key.at(0);
    }
    return QChar('#');
}
#include "qml_tools.h"

#include <QDateTime>
#include <QLocale>

QmlTools::QmlTools(QObject* parent)
    : QObject(parent)
{
}

QString QmlTools::modeSvg(int mode) const
{
    switch (mode) {
    case 1:
        return QStringLiteral("qrc:/img/single-loop.svg");
    case 2:
        return QStringLiteral("qrc:/img/shuffle.svg");
    default:
        return QStringLiteral("qrc:/img/list-loop.svg");
    }
}

QString QmlTools::formatProgress(qint64 ms) const
{
    if (ms <= 0)
        return QStringLiteral("0:00");

    int t = static_cast<int>(ms / 1000);
    int h = t / 3600;
    int m = (t % 3600) / 60;
    int s = t % 60;

    if (h > 0)
        return QStringLiteral("%1:%2:%3")
            .arg(h)
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0'));

    return QStringLiteral("%1:%2")
        .arg(m)
        .arg(s, 2, 10, QLatin1Char('0'));
}

QString QmlTools::formatRelative(qint64 timestamp) const
{
    if (timestamp <= 0)
        return QString();

    qint64 now = QDateTime::currentSecsSinceEpoch();
    qint64 diff = (now - timestamp) * 1000; // 转为毫秒

    if (diff < 3600000)
        return QStringLiteral("%1分钟前").arg(diff / 60000);
    if (diff < 86400000)
        return QStringLiteral("%1小时前").arg(diff / 3600000);
    if (diff < 604800000)
        return QStringLiteral("%1天前").arg(diff / 86400000);

    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
    return dt.toLocalTime().toString(QStringLiteral("yyyy/M/d"));
}

QString QmlTools::formatDateTime(qint64 timestamp) const
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
    return dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
}

int QmlTools::sectionPriority(const QString& key) const
{
    if (key.isEmpty())
        return 27;

    QChar c = key.at(0);
    if (c >= QLatin1Char('A') && c <= QLatin1Char('Z'))
        return c.unicode() - 65;
    if (c == QLatin1Char('0'))
        return 26;
    return 27;
}

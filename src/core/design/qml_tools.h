#ifndef QML_TOOLS_H
#define QML_TOOLS_H

#include <QObject>
#include <QString>

class QmlTools : public QObject
{
    Q_OBJECT

public:
    explicit QmlTools(QObject* parent = nullptr);

    Q_INVOKABLE QString modeSvg(int mode) const;
    Q_INVOKABLE QString formatProgress(qint64 ms) const;
    Q_INVOKABLE QString formatRelative(qint64 timestamp) const;
    Q_INVOKABLE QString formatDateTime(qint64 timestamp) const;
    Q_INVOKABLE int sectionPriority(const QString& key) const;
};

#endif

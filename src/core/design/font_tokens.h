#ifndef FONT_TOKENS_H
#define FONT_TOKENS_H

#include <QObject>
#include <QFont>
#include <QVariant>
#include <QHash>

// TODO: 若实现运行时主题切换，需将 CONSTANT 改为 NOTIFY 并发射信号
#define DS_INT(name, defaultVal)                                                      \
    Q_PROPERTY(int name READ name CONSTANT)                                          \
    int name() const { return m_values.value(QStringLiteral(#name), defaultVal).toInt(); }

class FontTokens : public QObject
{
    Q_OBJECT

public:
    explicit FontTokens(QObject* parent = nullptr);

    DS_INT(sizeXxs, 10)
    DS_INT(sizeXs,  12)
    DS_INT(sizeSm,  14)
    DS_INT(sizeMd,  16)
    DS_INT(sizeLg,  18)
    DS_INT(sizeXl,  20)
    DS_INT(sizeXxl, 24)
    DS_INT(sizeXxxl,32)

    DS_INT(weightLight,    300)
    DS_INT(weightNormal,   400)
    DS_INT(weightMedium,   500)
    DS_INT(weightDemiBold, 600)
    DS_INT(weightBold,     700)
    DS_INT(weightBlack,    900)

#undef DS_INT

    Q_INVOKABLE QFont createFont(int pixelSize, int weight = 400) const;

    void loadFromJson(const QJsonObject& section);
    void saveToJson(QJsonObject& section) const;
    void resetDefaults();

private:
    QHash<QString, QVariant> m_values;
};

#endif

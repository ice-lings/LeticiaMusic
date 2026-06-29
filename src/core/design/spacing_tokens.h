#ifndef SPACING_TOKENS_H
#define SPACING_TOKENS_H

#include <QObject>
#include <QVariant>
#include <QHash>
#include <QEasingCurve>

// TODO: 若实现运行时主题切换，需将 CONSTANT 改为 NOTIFY 并发射信号
#define DS_INT(name, defaultVal)                                                      \
    Q_PROPERTY(int name READ name CONSTANT)                                          \
    int name() const { return m_values.value(QStringLiteral(#name), defaultVal).toInt(); }

class SpacingTokens : public QObject
{
    Q_OBJECT

public:
    explicit SpacingTokens(QObject* parent = nullptr);

    DS_INT(gridUnit, 8)

    DS_INT(spaceXs, 8)
    DS_INT(spaceSm, 16)
    DS_INT(spaceMd, 24)
    DS_INT(spaceLg, 32)
    DS_INT(spaceXl, 48)

    DS_INT(radiusSm, 8)
    DS_INT(radiusMd, 12)

    DS_INT(durationFast, 100)
    DS_INT(durationNormal, 200)
    DS_INT(durationSlow, 300)
    DS_INT(durationHover, 100)

    DS_INT(controlHeightXs, 24)
    DS_INT(controlHeightSm, 32)
    DS_INT(controlHeightMd, 40)
    DS_INT(controlHeightLg, 48)
    DS_INT(controlHeightXl, 64)

    DS_INT(iconSizeXl, 64)
    DS_INT(iconSizeLg, 48)

    DS_INT(buttonRadius, 12)

    DS_INT(sidebarWidthSm, 240)
    DS_INT(sidebarWidthMd, 320)
    DS_INT(sidebarWidthLg, 400)

#undef DS_INT

    Q_INVOKABLE int getResponsiveControlHeight(const QString& size, int windowHeight = 0) const;
    Q_INVOKABLE QEasingCurve::Type getEasingCurve(const QString& curveType) const;

    void loadFromJson(const QJsonObject& section);
    void saveToJson(QJsonObject& section) const;
    void resetDefaults();

private:
    QHash<QString, QVariant> m_values;
};

#endif

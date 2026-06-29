#ifndef RESPONSIVE_TOKENS_H
#define RESPONSIVE_TOKENS_H

#include <QObject>
#include <QVariant>
#include <QHash>

#define DS_BREAKPOINT(name, defaultVal) DS_INT(name, defaultVal)

// TODO: 若实现运行时主题切换，需将 CONSTANT 改为 NOTIFY 并发射信号
#define DS_INT(name, defaultVal)                                                      \
    Q_PROPERTY(int name READ name CONSTANT)                                          \
    int name() const { return m_values.value(QStringLiteral(#name), defaultVal).toInt(); }

class ResponsiveTokens : public QObject
{
    Q_OBJECT

public:
    explicit ResponsiveTokens(QObject* parent = nullptr);

    DS_BREAKPOINT(breakpointXxs, 320)
    DS_BREAKPOINT(breakpointXs,  480)
    DS_BREAKPOINT(breakpointSm,  768)
    DS_BREAKPOINT(breakpointMd,  1024)
    DS_BREAKPOINT(breakpointLg,  1280)
    DS_BREAKPOINT(breakpointXl,  1440)
    DS_BREAKPOINT(breakpointXxl, 1920)
    DS_BREAKPOINT(breakpointXxxl,2560)

#undef DS_BREAKPOINT
#undef DS_INT

    Q_PROPERTY(qreal guiScale READ guiScale CONSTANT)
    qreal guiScale() const;

    Q_INVOKABLE bool isMobile() const;
    Q_INVOKABLE bool isDesktop() const;
    Q_INVOKABLE bool isTablet() const;
    Q_INVOKABLE bool isTouchDevice() const;

    Q_INVOKABLE int getDpiAdjustedSize(int baseSize, bool roundToGrid = false) const;
    Q_INVOKABLE double getDpiScale() const;

    void loadFromJson(const QJsonObject& section);
    void saveToJson(QJsonObject& section) const;
    void resetDefaults();

private:
    int screenWidth() const;
    int screenHeight() const;
    QHash<QString, QVariant> m_values;
};

#endif

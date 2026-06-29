#ifndef COLOR_TOKENS_H
#define COLOR_TOKENS_H

#include <QObject>
#include <QColor>
#include <QHash>

// TODO: 若实现运行时主题切换，需将 CONSTANT 改为 NOTIFY 并发射信号
#define DS_COLOR(name, defaultHex)                                                         \
    Q_PROPERTY(QColor name READ name CONSTANT)                                             \
    QColor name() const { return m_colors.value(QStringLiteral(#name), QColor(defaultHex)); }

class ColorTokens : public QObject
{
    Q_OBJECT

public:
    explicit ColorTokens(QObject* parent = nullptr);

    // ==================== 主色 ====================
    DS_COLOR(primary,       "#4a90e2")
    DS_COLOR(primaryDark,   "#3a7bb0")
    DS_COLOR(primaryLight,  "#5aa5f5")

    // ==================== 强调色 ====================
    DS_COLOR(accent,        "#ff6b6b")
    DS_COLOR(success,       "#2ecc71")
    DS_COLOR(warning,       "#f39c12")
    DS_COLOR(error,         "#e74c3c")

    // ==================== 背景色 ====================
    DS_COLOR(background,    "#2d2d37")
    DS_COLOR(surface,       "#1a1a27")
    DS_COLOR(card,          "#13131a")
    DS_COLOR(divider,       "#3a3a4a")

    // ==================== 文字色 ====================
    DS_COLOR(textPrimary,   "#ffffff")
    DS_COLOR(textSecondary, "#b3aaaeee")
    DS_COLOR(textDisabled,  "#888888")
    DS_COLOR(textHint,      "#666666")

    // ==================== 播放器色 ====================
    DS_COLOR(playingIndicator,  "#4a90e2")
    DS_COLOR(progressFill,      "#4a90e2")
    DS_COLOR(progressTrack,     "#3a3a44")
    DS_COLOR(volumeFill,        "#2ecc71")

    // ==================== 特殊色 ====================
    DS_COLOR(favorite,      "#ff6b6b")

    // ==================== 按钮色 ====================
    DS_COLOR(buttonNormal,      "#3a3a44")
    DS_COLOR(buttonHovered,     "#4a4a54")
    DS_COLOR(buttonPressed,     "#2a2a34")
    DS_COLOR(buttonDisabled,    "#2a2a34")

    // ==================== 列表项色 ====================
    DS_COLOR(listItemNormal,    "transparent")
    DS_COLOR(listItemHovered,   "#19ffffff")
    DS_COLOR(listItemPressed,   "#4dffffff")
    DS_COLOR(listItemSelected,  "#4dffffff")
    DS_COLOR(listItemAlternate, "#0dffffff")

    // ==================== 图标色 ====================
    DS_COLOR(iconNormal,    "#ffffff")
    DS_COLOR(iconHovered,   "#4a90e2")
    DS_COLOR(iconDisabled,  "#666666")

#undef DS_COLOR

    // ==================== Alpha 常量 ====================
    Q_PROPERTY(double alphaTransparent READ alphaTransparent CONSTANT)
    Q_PROPERTY(double alphaLow READ alphaLow CONSTANT)
    Q_PROPERTY(double alphaMedium READ alphaMedium CONSTANT)
    Q_PROPERTY(double alphaHigh READ alphaHigh CONSTANT)
    Q_PROPERTY(double alphaOpaque READ alphaOpaque CONSTANT)

    double alphaTransparent() const { return 0.0; }
    double alphaLow() const         { return 0.1; }
    double alphaMedium() const      { return 0.3; }
    double alphaHigh() const        { return 0.6; }
    double alphaOpaque() const      { return 1.0; }

    // ==================== 辅助函数 ====================
    Q_INVOKABLE QColor withAlpha(const QColor& base, double alpha) const;

    void loadFromJson(const QJsonObject& colorsSection);
    void saveToJson(QJsonObject& colorsSection) const;
    void resetDefaults();

private:
    QHash<QString, QColor> m_colors;
};

#endif

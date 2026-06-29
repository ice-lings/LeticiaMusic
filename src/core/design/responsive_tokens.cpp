#include "responsive_tokens.h"
#include "design_system_config.h"

#include <QGuiApplication>
#include <QScreen>
#include <QJsonObject>
#include <QtMath>
#include <QtGlobal>

ResponsiveTokens::ResponsiveTokens(QObject* parent)
    : QObject(parent)
{
}

qreal ResponsiveTokens::guiScale() const
{
    int w = screenWidth();
    int h = screenHeight();
    return qMin(w, h) / 360.0;
}

int ResponsiveTokens::screenWidth() const
{
    QScreen* screen = QGuiApplication::primaryScreen();
    return screen ? screen->size().width() : 1920;
}

int ResponsiveTokens::screenHeight() const
{
    QScreen* screen = QGuiApplication::primaryScreen();
    return screen ? screen->size().height() : 1080;
}

bool ResponsiveTokens::isMobile() const
{
    int w = screenWidth();
    return w <= breakpointMd();
}

bool ResponsiveTokens::isDesktop() const
{
    return !isMobile();
}

bool ResponsiveTokens::isTablet() const
{
    int w = screenWidth();
    return w > breakpointSm() && w <= breakpointMd();
}

bool ResponsiveTokens::isTouchDevice() const
{
#ifdef Q_OS_ANDROID
    return true;
#elif defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

int ResponsiveTokens::getDpiAdjustedSize(int baseSize, bool roundToGrid) const
{
    double scale = getDpiScale();
    if (scale <= 1.0) return baseSize;
    int result = qRound(baseSize * scale);
    if (roundToGrid) {
        result = qRound(static_cast<double>(result) / 8) * 8;
    }
    return result;
}

double ResponsiveTokens::getDpiScale() const
{
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return 1.0;
    return screen->physicalDotsPerInch() / 96.0;
}

void ResponsiveTokens::loadFromJson(const QJsonObject& section)
{
    for (auto it = section.begin(); it != section.end(); ++it) {
        if (m_values.contains(it.key()) && it.value().isDouble()) {
            m_values[it.key()] = it.value().toInt();
        }
    }
}

void ResponsiveTokens::saveToJson(QJsonObject& section) const
{
    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        section[it.key()] = it.value().toInt();
    }
}

void ResponsiveTokens::resetDefaults()
{
    QJsonObject defaults = DesignSystemConfig::defaults();
    QJsonObject defaultSection = defaults.value("responsive").toObject();
    m_values.clear();
    for (auto it = defaultSection.begin(); it != defaultSection.end(); ++it) {
        if (it.value().isDouble()) {
            m_values[it.key()] = it.value().toInt();
        }
    }
}

#include "spacing_tokens.h"
#include "design_system_config.h"

#include <QJsonObject>
#include <QtGlobal>
#include <QtMath>

SpacingTokens::SpacingTokens(QObject* parent)
    : QObject(parent)
{
}

int SpacingTokens::getResponsiveControlHeight(const QString& size, int windowHeight) const
{
    int baseHeight;
    if (size == "xs") baseHeight = controlHeightXs();
    else if (size == "sm") baseHeight = controlHeightSm();
    else if (size == "md") baseHeight = controlHeightMd();
    else if (size == "lg") baseHeight = controlHeightLg();
    else if (size == "xl") baseHeight = controlHeightXl();
    else baseHeight = controlHeightMd();

    if (windowHeight > 0) {
        double scaleFactor = static_cast<double>(windowHeight) / 800.0;
        scaleFactor = qBound(0.8, scaleFactor, 1.2);
        return qRound(baseHeight * scaleFactor);
    }

    return baseHeight;
}

QEasingCurve::Type SpacingTokens::getEasingCurve(const QString& curveType) const
{
    if (curveType == "linear") return QEasingCurve::Linear;
    if (curveType == "easeIn") return QEasingCurve::InQuad;
    if (curveType == "easeOut") return QEasingCurve::OutQuad;
    if (curveType == "easeInOut") return QEasingCurve::InOutQuad;
    if (curveType == "bounce") return QEasingCurve::OutBounce;
    return QEasingCurve::InOutQuad;
}

void SpacingTokens::loadFromJson(const QJsonObject& section)
{
    for (auto it = section.begin(); it != section.end(); ++it) {
        if (m_values.contains(it.key()) && it.value().isDouble()) {
            m_values[it.key()] = it.value().toInt();
        }
    }
}

void SpacingTokens::saveToJson(QJsonObject& section) const
{
    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        section[it.key()] = it.value().toInt();
    }
}

void SpacingTokens::resetDefaults()
{
    QJsonObject defaults = DesignSystemConfig::defaults();
    QJsonObject defaultSpacing = defaults.value("spacing").toObject();
    m_values.clear();
    for (auto it = defaultSpacing.begin(); it != defaultSpacing.end(); ++it) {
        if (it.value().isDouble()) {
            m_values[it.key()] = it.value().toInt();
        }
    }
}

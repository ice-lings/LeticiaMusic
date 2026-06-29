#include "color_tokens.h"
#include "design_system_config.h"

#include <QJsonObject>
#include <QtGlobal>

ColorTokens::ColorTokens(QObject* parent)
    : QObject(parent)
{
}

QColor ColorTokens::withAlpha(const QColor& base, double alpha) const
{
    QColor result = base;
    result.setAlphaF(qBound(0.0, alpha, 1.0));
    return result;
}

void ColorTokens::loadFromJson(const QJsonObject& colorsSection)
{
    for (auto it = colorsSection.begin(); it != colorsSection.end(); ++it) {
        if (m_colors.contains(it.key())) {
            QColor c(it.value().toString());
            if (c.isValid()) {
                m_colors[it.key()] = c;
            }
        }
    }
}

void ColorTokens::saveToJson(QJsonObject& colorsSection) const
{
    for (auto it = m_colors.begin(); it != m_colors.end(); ++it) {
        if (it.value().isValid()) {
            colorsSection[it.key()] = it.value().name(QColor::HexArgb);
        }
    }
}

void ColorTokens::resetDefaults()
{
    QJsonObject defaults = DesignSystemConfig::defaults();
    QJsonObject defaultColors = defaults.value("colors").toObject();
    m_colors.clear();
    for (auto it = defaultColors.begin(); it != defaultColors.end(); ++it) {
        QColor c(it.value().toString());
        if (c.isValid()) {
            m_colors[it.key()] = c;
        }
    }
}

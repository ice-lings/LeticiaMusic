#include "font_tokens.h"
#include "design_system_config.h"

#include <QJsonObject>

FontTokens::FontTokens(QObject* parent)
    : QObject(parent)
{
}

QFont FontTokens::createFont(int pixelSize, int weight) const
{
    QFont font;
    font.setPixelSize(pixelSize);
    font.setWeight(QFont::Weight(weight));
    return font;
}

void FontTokens::loadFromJson(const QJsonObject& section)
{
    for (auto it = section.begin(); it != section.end(); ++it) {
        if (m_values.contains(it.key()) && it.value().isDouble()) {
            m_values[it.key()] = it.value().toInt();
        }
    }
}

void FontTokens::saveToJson(QJsonObject& section) const
{
    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        section[it.key()] = it.value().toInt();
    }
}

void FontTokens::resetDefaults()
{
    QJsonObject defaults = DesignSystemConfig::defaults();
    QJsonObject defaultSection = defaults.value("fonts").toObject();
    m_values.clear();
    for (auto it = defaultSection.begin(); it != defaultSection.end(); ++it) {
        if (it.value().isDouble()) {
            m_values[it.key()] = it.value().toInt();
        }
    }
}

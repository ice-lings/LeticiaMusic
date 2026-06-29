#include "design_system.h"
#include "color_tokens.h"
#include "spacing_tokens.h"
#include "responsive_tokens.h"
#include "font_tokens.h"
#include "design_system_config.h"

DesignSystem::DesignSystem(QObject* parent)
    : QObject(parent)
    , m_colors(new ColorTokens(this))
    , m_spacing(new SpacingTokens(this))
    , m_responsive(new ResponsiveTokens(this))
    , m_fonts(new FontTokens(this))
{
    QJsonObject json = DesignSystemConfig::load();
    m_colors->loadFromJson(json.value("colors").toObject());
    m_spacing->loadFromJson(json.value("spacing").toObject());
    m_responsive->loadFromJson(json.value("responsive").toObject());
    m_fonts->loadFromJson(json.value("fonts").toObject());

    DesignSystemConfig::save(json);
}

void DesignSystem::save()
{
    QJsonObject json = DesignSystemConfig::load();

    QJsonObject cs; m_colors->saveToJson(cs); json["colors"] = cs;
    QJsonObject ss; m_spacing->saveToJson(ss); json["spacing"] = ss;
    QJsonObject rs; m_responsive->saveToJson(rs); json["responsive"] = rs;
    QJsonObject fs; m_fonts->saveToJson(fs); json["fonts"] = fs;

    DesignSystemConfig::save(json);
}

void DesignSystem::resetAll()
{
    m_colors->resetDefaults();
    m_spacing->resetDefaults();
    m_responsive->resetDefaults();
    m_fonts->resetDefaults();

    QJsonObject json = DesignSystemConfig::defaults();
    DesignSystemConfig::save(json);
}

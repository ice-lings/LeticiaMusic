#ifndef DESIGN_SYSTEM_CONFIG_H
#define DESIGN_SYSTEM_CONFIG_H

#include <QObject>
#include <QJsonObject>

class DesignSystemConfig
{
public:
    static QString filePath();

    static QJsonObject load();

    static bool save(const QJsonObject& json);

    static QJsonObject defaults();

    static constexpr const char* kFileName = "design_system.json";
    static constexpr int kVersion = 1;
};

#endif

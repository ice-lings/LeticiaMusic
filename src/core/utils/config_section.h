#ifndef CONFIG_SECTION_H
#define CONFIG_SECTION_H

#include <QJsonObject>
#include <QString>

class IConfigSection
{
public:
    virtual ~IConfigSection() = default;
    virtual QString sectionKey() const = 0;
    virtual QJsonObject serialize() const = 0;
    virtual void deserialize(const QJsonObject& json) = 0;
};

#endif

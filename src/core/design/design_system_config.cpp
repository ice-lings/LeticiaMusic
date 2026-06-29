#include "design_system_config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>

QString DesignSystemConfig::filePath()
{
#ifdef Q_OS_ANDROID
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString dir = QCoreApplication::applicationDirPath();
#endif
    return dir + "/" + kFileName;
}

QJsonObject DesignSystemConfig::load()
{
    QFile file(filePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        file.close();
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.value("version").toInt() == kVersion) {
                return obj;
            }
        }
    }
    return defaults();
}

bool DesignSystemConfig::save(const QJsonObject& json)
{
    QString path = filePath();
    QString tmpPath = path + ".tmp";

    QFile file(tmpPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    file.flush();
    file.close();

    QFile::remove(path);
    return QFile::rename(tmpPath, path);
}

QJsonObject DesignSystemConfig::defaults()
{
    QJsonObject root;
    root["version"] = kVersion;

    QJsonObject colors;
    colors["primary"]             = "#4a90e2";
    colors["primaryDark"]         = "#3a7bb0";
    colors["primaryLight"]        = "#5aa5f5";
    colors["accent"]              = "#ff6b6b";
    colors["success"]             = "#2ecc71";
    colors["warning"]             = "#f39c12";
    colors["error"]               = "#e74c3c";
    colors["background"]          = "#2d2d37";
    colors["surface"]             = "#1a1a27";
    colors["card"]                = "#13131a";
    colors["divider"]             = "#3a3a4a";
    colors["textPrimary"]         = "#ffffff";
    colors["textSecondary"]       = "#b3aaaeee";
    colors["textDisabled"]        = "#888888";
    colors["textHint"]            = "#666666";
    colors["playingIndicator"]    = "#4a90e2";
    colors["progressFill"]        = "#4a90e2";
    colors["progressTrack"]       = "#3a3a44";
    colors["volumeFill"]          = "#2ecc71";
    colors["favorite"]            = "#ff6b6b";
    colors["buttonNormal"]        = "#3a3a44";
    colors["buttonHovered"]       = "#4a4a54";
    colors["buttonPressed"]       = "#2a2a34";
    colors["buttonDisabled"]      = "#2a2a34";
    colors["listItemNormal"]      = "transparent";
    colors["listItemHovered"]     = "#19ffffff";
    colors["listItemPressed"]     = "#4dffffff";
    colors["listItemSelected"]    = "#4dffffff";
    colors["listItemAlternate"]   = "#0dffffff";
    colors["iconNormal"]          = "#ffffff";
    colors["iconHovered"]         = "#4a90e2";
    colors["iconDisabled"]        = "#666666";

    root["colors"] = colors;

    QJsonObject spacing;
    spacing["gridUnit"]         = 8;
    spacing["spaceXs"]          = 8;
    spacing["spaceSm"]          = 16;
    spacing["spaceMd"]          = 24;
    spacing["spaceLg"]          = 32;
    spacing["spaceXl"]          = 48;
    spacing["radiusSm"]         = 8;
    spacing["radiusMd"]         = 12;
    spacing["durationFast"]     = 100;
    spacing["durationNormal"]   = 200;
    spacing["durationSlow"]     = 300;
    spacing["durationHover"]    = 100;
    spacing["controlHeightXs"]  = 24;
    spacing["controlHeightSm"]  = 32;
    spacing["controlHeightMd"]  = 40;
    spacing["controlHeightLg"]  = 48;
    spacing["controlHeightXl"]  = 64;
    spacing["iconSizeXl"]       = 64;
    spacing["iconSizeLg"]       = 48;
    spacing["buttonRadius"]     = 12;
    spacing["sidebarWidthSm"]   = 240;
    spacing["sidebarWidthMd"]   = 320;
    spacing["sidebarWidthLg"]   = 400;
    root["spacing"] = spacing;

    QJsonObject responsive;
    responsive["breakpointXxs"]  = 320;
    responsive["breakpointXs"]   = 480;
    responsive["breakpointSm"]   = 768;
    responsive["breakpointMd"]   = 1024;
    responsive["breakpointLg"]   = 1280;
    responsive["breakpointXl"]   = 1440;
    responsive["breakpointXxl"]  = 1920;
    responsive["breakpointXxxl"] = 2560;
    root["responsive"] = responsive;

    QJsonObject fonts;
    fonts["sizeXxs"]      = 10;
    fonts["sizeXs"]       = 12;
    fonts["sizeSm"]       = 14;
    fonts["sizeMd"]       = 16;
    fonts["sizeLg"]       = 18;
    fonts["sizeXl"]       = 20;
    fonts["sizeXxl"]      = 24;
    fonts["sizeXxxl"]     = 32;
    fonts["weightLight"]    = 300;
    fonts["weightNormal"]   = 400;
    fonts["weightMedium"]   = 500;
    fonts["weightDemiBold"] = 600;
    fonts["weightBold"]     = 700;
    fonts["weightBlack"]    = 900;
    root["fonts"] = fonts;

    return root;
}

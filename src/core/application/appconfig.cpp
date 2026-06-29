#include "appconfig.h"
#include "../utils/logger.h"
#include "../utils/platform_path_helper.h"
#include "../utils/config_section.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QCryptographicHash>

void AppConfig::loadOrInit()
{
    auto& inst = instance();
    QString filePath = getConfigFilePath();

    auto useDefaultConfig = [&inst](){
        inst.applyDefaults();
        inst.save();
    };

    QFile file(filePath);
    if (!file.exists()) {
        Log.debug("Config file not found, using default config");
        useDefaultConfig();
        return;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) {
            Log.warning("JSON parse error:", error.errorString(), "using default config");
            useDefaultConfig();
            return;
        }

        if (!doc.isObject()) {
            Log.warning("Invalid config format, using default config");
            useDefaultConfig();
            return;
        }

        Log.debug("AppConfig loaded from:", filePath);
        inst.fromJsonInternal(doc.object());
    } else {
        Log.warning("Failed to open config file:", filePath, ", using default config");
        useDefaultConfig();
    }
}

void AppConfig::applyDefaults()
{
    m_databasePath = "";
    m_musicScanDirectories = QStringList() << QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    m_autoScanMusicOnStartup = true;
    m_resumePlaybackOnStartup = false;
    m_lastVolume = 80;
    m_minFileSizeKB = 100;
    m_minDurationSec = 10;
    defaultViewModelId = "local_music";
    enableFileLogging = true;
    enableConsoleLogging = true;
    sync.deviceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString AppConfig::getConfigFilePath()
{
#ifdef Q_OS_ANDROID
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString configDir = QCoreApplication::applicationDirPath();
#endif
    return configDir + "/config.json";
}

bool AppConfig::save() const
{
    QString filePath = getConfigFilePath();
    QJsonObject root;

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();
        QJsonParseError error;
        QJsonDocument existing = QJsonDocument::fromJson(data, &error);
        if (error.error == QJsonParseError::NoError && existing.isObject()) {
            root = existing.object();
        }
    }

    QJsonObject ours = toJson();
    for (auto it = ours.begin(); it != ours.end(); ++it) {
        root[it.key()] = it.value();
    }

    QSaveFile out(filePath);
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        if (out.commit()) {
            Log.debug("AppConfig saved to:", filePath);
            return true;
        } else {
            Log.warning("AppConfig commit failed:", filePath, "error:", out.errorString());
            return false;
        }
    } else {
        Log.warning("Failed to save config to:", filePath);
        return false;
    }
}

bool AppConfig::saveSection(const IConfigSection* section)
{
    QString filePath = getConfigFilePath();
    QJsonObject root;

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();
        QJsonParseError error;
        QJsonDocument existing = QJsonDocument::fromJson(data, &error);
        if (error.error == QJsonParseError::NoError && existing.isObject()) {
            root = existing.object();
        }
    }

    root[section->sectionKey()] = section->serialize();

    QSaveFile out(filePath);
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        return out.commit();
    }
    return false;
}

bool AppConfig::loadSection(IConfigSection* section) const
{
    QString filePath = getConfigFilePath();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains(section->sectionKey())) {
        return false;
    }

    section->deserialize(root[section->sectionKey()].toObject());
    return true;
}

QJsonObject AppConfig::toJson() const
{
    QJsonObject json;
    json["databasePath"] = m_databasePath;
    json["musicScanDirectories"] = stringListToJsonArray(m_musicScanDirectories);
    json["autoScanMusicOnStartup"] = m_autoScanMusicOnStartup;
    json["resumePlaybackOnStartup"] = m_resumePlaybackOnStartup;
    json["lastVolume"] = m_lastVolume;
    json["defaultViewModelId"] = defaultViewModelId;
    json["enableFileLogging"] = enableFileLogging;
    json["enableConsoleLogging"] = enableConsoleLogging;

    QJsonArray dupArr;
    for (const auto& h : m_resolvedDuplicateHashes) {
        dupArr.append(h);
    }
    json["m_resolvedDuplicateHashes"] = dupArr;

    json["minFileSizeKB"] = m_minFileSizeKB;
    json["minDurationSec"] = m_minDurationSec;
    json["lastCoverPath"] = m_lastCoverPath;

#ifdef Q_OS_ANDROID
    QJsonArray sfArr;
    for (const auto& sf : scanFolders) {
        QJsonObject obj;
        obj["path"] = sf.path;
        obj["musicCount"] = sf.musicCount;
        obj["enabled"] = sf.enabled;
        sfArr.append(obj);
    }
    json["scanFolders"] = sfArr;
#endif

    QJsonObject syncObj;
    syncObj["enabled"] = sync.enabled;
    syncObj["nasHost"] = sync.nasHost;
    syncObj["nasPort"] = sync.nasPort;
    syncObj["nasUser"] = sync.nasUser;
    syncObj["nasPassword"] = encryptPassword(sync.nasPassword, sync.deviceId);
    syncObj["nasSyncRoot"] = sync.nasSyncRoot;
    syncObj["deviceId"] = sync.deviceId;
    syncObj["lastSyncTime"] = sync.lastSyncTime;
    json["sync"] = syncObj;

    QJsonObject cpObj;
    for (auto it = canonicalPathMap.constBegin(); it != canonicalPathMap.constEnd(); ++it)
        cpObj[it.key()] = it.value();
    json["canonicalPathMap"] = cpObj;

#ifndef Q_OS_ANDROID
    json["downloadPath"] = m_downloadPath;
#endif

    return json;
}

void AppConfig::fromJsonInternal(const QJsonObject& json)
{
    if (json.contains("databasePath")) {
        m_databasePath = json["databasePath"].toString();
    } else {
        m_databasePath = "";
    }

    if (json.contains("musicScanDirectories") && !json["musicScanDirectories"].toArray().isEmpty()) {
        m_musicScanDirectories = jsonArrayToStringList(json["musicScanDirectories"].toArray());
    } else {
        m_musicScanDirectories = QStringList() << QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    }

    m_autoScanMusicOnStartup = json["autoScanMusicOnStartup"].toBool(true);
    m_resumePlaybackOnStartup = json["resumePlaybackOnStartup"].toBool(false);
    m_lastVolume = json["lastVolume"].toInt(80);
    defaultViewModelId = json["defaultViewModelId"].toString("local_music");
    enableFileLogging = json["enableFileLogging"].toBool(true);
    enableConsoleLogging = json["enableConsoleLogging"].toBool(true);

    m_minFileSizeKB = json["minFileSizeKB"].toInt(100);
    m_minDurationSec = json["minDurationSec"].toInt(10);
    m_lastCoverPath = json["lastCoverPath"].toString("");

#ifdef Q_OS_ANDROID
    if (json.contains("scanFolders") && json["scanFolders"].isArray()) {
        QJsonArray sfArr = json["scanFolders"].toArray();
        scanFolders.clear();
        for (const auto& v : sfArr) {
            QJsonObject obj = v.toObject();
            ScanFolderEntry sf;
            sf.path = obj["path"].toString();
            sf.musicCount = obj["musicCount"].toInt(0);
            sf.enabled = obj["enabled"].toBool(true);
            scanFolders.append(sf);
        }
    }
#endif

    if (json.contains("m_resolvedDuplicateHashes") && json["m_resolvedDuplicateHashes"].isArray()) {
        QJsonArray dupArr = json["m_resolvedDuplicateHashes"].toArray();
        m_resolvedDuplicateHashes.clear();
        for (const auto& v : dupArr) {
            m_resolvedDuplicateHashes.insert(v.toString());
        }
    }

    if (json.contains("canonicalPathMap") && json["canonicalPathMap"].isObject()) {
        QJsonObject cpObj = json["canonicalPathMap"].toObject();
        canonicalPathMap.clear();
        for (auto it = cpObj.begin(); it != cpObj.end(); ++it)
            canonicalPathMap[it.key()] = it.value().toString();
    }

    if (json.contains("sync") && json["sync"].isObject()) {
        QJsonObject syncObj = json["sync"].toObject();
        sync.enabled = syncObj.value("enabled").toBool(false);
        sync.nasHost = syncObj.value("nasHost").toString();
        sync.nasPort = syncObj.value("nasPort").toInt(21);
        sync.nasUser = syncObj.value("nasUser").toString("admin");
        sync.nasSyncRoot = syncObj.value("nasSyncRoot").toString("MUSIC/LeticiaMusic");
        sync.deviceId = syncObj.value("deviceId").toString();
        if (sync.deviceId.isEmpty()) {
            sync.deviceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }
        sync.nasPassword = decryptPassword(
            syncObj.value("nasPassword").toString(), sync.deviceId);
        sync.lastSyncTime = syncObj.value("lastSyncTime").toVariant().toLongLong();
    }

#ifndef Q_OS_ANDROID
    if (json.contains("downloadPath")) {
        m_downloadPath = json["downloadPath"].toString();
    }
#endif
}

#ifndef Q_OS_ANDROID
void AppConfig::setDownloadPath(const QString& v)
{
    m_downloadPath = v;
    PlatformPathHelper::setDownloadPath(v);
    save();
    emit configChanged();
}
#endif

void AppConfig::addMusicScanDirectory(const QString& path)
{
    if (!path.isEmpty() && !m_musicScanDirectories.contains(path)) {
        m_musicScanDirectories.append(path);
        save();
        Log.info(QString("Added music scan directory: %1").arg(path));
    }
}

void AppConfig::removeMusicScanDirectory(const QString& path)
{
    m_musicScanDirectories.removeAll(path);
    save();
}

QString AppConfig::getDownloadPath() const
{
#ifdef Q_OS_ANDROID
    return PlatformPathHelper::downloadDir();
#else
    return m_downloadPath.isEmpty() ? getDefaultDownloadPath() : m_downloadPath;
#endif
}

QString AppConfig::getDefaultDownloadPath() const
{
#ifdef Q_OS_ANDROID
    return PlatformPathHelper::downloadDir();
#else
    return QCoreApplication::applicationDirPath() + "/Music";
#endif
}

QString AppConfig::getAppVersion() const
{
    return "1.0.0";
}

QVariantMap AppConfig::getSyncConfig() const
{
    QVariantMap cfg;
    cfg["enabled"] = sync.enabled;
    cfg["nasHost"] = sync.nasHost;
    cfg["nasPort"] = sync.nasPort;
    cfg["nasUser"] = sync.nasUser;
    cfg["nasPassword"] = sync.nasPassword;
    cfg["nasSyncRoot"] = sync.nasSyncRoot;
    cfg["deviceId"] = sync.deviceId;
    cfg["lastSyncTime"] = static_cast<qint64>(sync.lastSyncTime);
    return cfg;
}

void AppConfig::setSyncConfig(const QVariantMap& cfg)
{
    if (cfg.contains("enabled"))       sync.enabled = cfg["enabled"].toBool();
    if (cfg.contains("nasHost"))       sync.nasHost = cfg["nasHost"].toString();
    if (cfg.contains("nasPort"))       sync.nasPort = cfg["nasPort"].toInt();
    if (cfg.contains("nasUser"))       sync.nasUser = cfg["nasUser"].toString();
    if (cfg.contains("nasPassword"))   sync.nasPassword = cfg["nasPassword"].toString();
    if (cfg.contains("nasSyncRoot"))   sync.nasSyncRoot = cfg["nasSyncRoot"].toString();
    save();
}

QByteArray AppConfig::deriveKey(const QString& deviceId)
{
    QByteArray salt = QByteArrayLiteral("LeticiaMusic_v1_salt");
    QByteArray input = deviceId.toUtf8() + salt;
    QByteArray hash = QCryptographicHash::hash(input, QCryptographicHash::Sha256);
    return hash.left(32);
}

QString AppConfig::encryptPassword(const QString& plain, const QString& deviceId)
{
    if (plain.isEmpty()) return "";
    QByteArray key = deriveKey(deviceId);
    QByteArray data = plain.toUtf8();
    QByteArray encrypted;
    for (int i = 0; i < data.size(); i++) {
        encrypted.append(data[i] ^ key[i % key.size()]);
    }
    return QString::fromLatin1(encrypted.toBase64());
}

QString AppConfig::decryptPassword(const QString& cipher, const QString& deviceId)
{
    if (cipher.isEmpty()) return "";
    QByteArray key = deriveKey(deviceId);
    QByteArray data = QByteArray::fromBase64(cipher.toLatin1());
    QByteArray decrypted;
    for (int i = 0; i < data.size(); i++) {
        decrypted.append(data[i] ^ key[i % key.size()]);
    }
    return QString::fromUtf8(decrypted);
}

QJsonArray AppConfig::stringListToJsonArray(const QStringList& list)
{
    QJsonArray array;
    for (const QString& str : list) {
        array.append(str);
    }
    return array;
}

QStringList AppConfig::jsonArrayToStringList(const QJsonArray& array)
{
    QStringList list;
    for (const QJsonValue& value : array) {
        list.append(value.toString());
    }
    return list;
}

#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <QDateTime>
#include <QSet>
#include <QMap>
#include <QVariantList>
#include <QVariantMap>

#include "../utils/singleton_holder.h"
#include "../sync/sync_config.h"

class AppConfig : public QObject, public SingletonHolder<AppConfig>
{
    Q_OBJECT
    friend class SingletonHolder<AppConfig>;
    AppConfig(QObject* parent = nullptr) : QObject(parent) {}

public:
    struct ScanFolderEntry {
        QString path;
        int musicCount = 0;
        bool enabled = true;
    };

    Q_PROPERTY(QString databasePath READ databasePath WRITE setDatabasePath NOTIFY configChanged)
    Q_PROPERTY(QStringList musicScanDirectories READ musicScanDirectories WRITE setMusicScanDirectories NOTIFY configChanged)
    Q_PROPERTY(bool autoScanMusicOnStartup READ autoScanMusicOnStartup WRITE setAutoScanMusicOnStartup NOTIFY configChanged)
    Q_PROPERTY(bool resumePlaybackOnStartup READ resumePlaybackOnStartup WRITE setResumePlaybackOnStartup NOTIFY configChanged)
    Q_PROPERTY(int lastVolume READ lastVolume WRITE setLastVolume NOTIFY configChanged)
    Q_PROPERTY(int minFileSizeKB READ minFileSizeKB WRITE setMinFileSizeKB NOTIFY configChanged)
    Q_PROPERTY(int minDurationSec READ minDurationSec WRITE setMinDurationSec NOTIFY configChanged)
    Q_PROPERTY(QString lastCoverPath READ lastCoverPath WRITE setLastCoverPath NOTIFY configChanged)
#ifndef Q_OS_ANDROID
    Q_PROPERTY(QString downloadPath READ downloadPath WRITE setDownloadPath NOTIFY configChanged)
#endif

    static void loadOrInit();
    static AppConfig& instance() { return get_instance(); }

    QString databasePath() const               { return m_databasePath; }
    QStringList musicScanDirectories() const   { return m_musicScanDirectories; }
    bool autoScanMusicOnStartup() const        { return m_autoScanMusicOnStartup; }
    bool resumePlaybackOnStartup() const       { return m_resumePlaybackOnStartup; }
    int lastVolume() const                     { return m_lastVolume; }
    int minFileSizeKB() const                  { return m_minFileSizeKB; }
    int minDurationSec() const                 { return m_minDurationSec; }
    QString lastCoverPath() const               { return m_lastCoverPath; }
#ifndef Q_OS_ANDROID
    QString downloadPath() const               { return m_downloadPath; }
#endif

    void setDatabasePath(const QString& v)           { m_databasePath = v; save(); emit configChanged(); }
    void setMusicScanDirectories(const QStringList& v) { m_musicScanDirectories = v; save(); emit configChanged(); }
    void setAutoScanMusicOnStartup(bool v)           { m_autoScanMusicOnStartup = v; save(); emit configChanged(); }
    void setResumePlaybackOnStartup(bool v)          { m_resumePlaybackOnStartup = v; save(); emit configChanged(); }
    void setLastVolume(int v)                        { m_lastVolume = v; save(); emit configChanged(); }
    void setMinFileSizeKB(int v)                     { m_minFileSizeKB = v; save(); emit configChanged(); }
    void setMinDurationSec(int v)                    { m_minDurationSec = v; save(); emit configChanged(); }
    void setLastCoverPath(const QString& v)          { m_lastCoverPath = v; save(); emit configChanged(); }
#ifndef Q_OS_ANDROID
    void setDownloadPath(const QString& v);
#endif

    Q_INVOKABLE void addMusicScanDirectory(const QString& path);
    Q_INVOKABLE void removeMusicScanDirectory(const QString& path);
    Q_INVOKABLE QString getDownloadPath() const;
    Q_INVOKABLE QString getDefaultDownloadPath() const;
    Q_INVOKABLE QString getAppVersion() const;
    Q_INVOKABLE QVariantMap getSyncConfig() const;
    Q_INVOKABLE void setSyncConfig(const QVariantMap& cfg);
    bool saveSection(const class IConfigSection* section);
    bool loadSection(class IConfigSection* section) const;

#ifdef Q_OS_ANDROID
    QList<ScanFolderEntry> scanFolders;
#endif

    const QSet<QString>& resolvedDuplicateHashes() const { return m_resolvedDuplicateHashes; }
    void addResolvedDuplicateHash(const QString& hash) { m_resolvedDuplicateHashes.insert(hash); save(); }

    QString canonicalPath(const QString& fileHash) const { return canonicalPathMap.value(fileHash); }
    bool hasCanonicalPath(const QString& fileHash) const { return canonicalPathMap.contains(fileHash); }
    void setCanonicalPathEntry(const QString& fileHash, const QString& path) { canonicalPathMap[fileHash] = path; save(); }

    const SyncConfig& syncConfig() const { return sync; }
    void setLastSyncTime(qint64 ts) { sync.lastSyncTime = ts; save(); }

    const QString& viewModelId() const { return defaultViewModelId; }
    bool isFileLoggingEnabled() const { return enableFileLogging; }
    bool isConsoleLoggingEnabled() const { return enableConsoleLogging; }

    bool save() const;
    static QString getConfigFilePath();

signals:
    void configChanged();

private:
    QSet<QString> m_resolvedDuplicateHashes;
    QMap<QString, QString> canonicalPathMap;
    QString defaultViewModelId;
    bool enableFileLogging = true;
    bool enableConsoleLogging = true;
    SyncConfig sync;

    QString m_databasePath;
    QStringList m_musicScanDirectories;
    bool m_autoScanMusicOnStartup = true;
    bool m_resumePlaybackOnStartup = true;
    int m_lastVolume = 80;
    int m_minFileSizeKB = 100;
    int m_minDurationSec = 10;
    QString m_lastCoverPath;
#ifndef Q_OS_ANDROID
    QString m_downloadPath;
#endif

    QJsonObject toJson() const;
    void fromJsonInternal(const QJsonObject& json);
    void applyDefaults();

    static QByteArray deriveKey(const QString& deviceId);
    static QString encryptPassword(const QString& plain, const QString& deviceId);
    static QString decryptPassword(const QString& cipher, const QString& deviceId);
    static QStringList jsonArrayToStringList(const QJsonArray& array);
    static QJsonArray stringListToJsonArray(const QStringList& list);
};

#endif

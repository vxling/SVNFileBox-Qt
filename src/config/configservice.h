#ifndef CONFIGSERVICE_H
#define CONFIGSERVICE_H

#include <QObject>
#include <QString>
#include <QVariantList>

class CredentialStore;

struct Repository {
    QString name;
    QString url;
    QString localPath;
    QString username;
    QString password;  // deprecated: passwords now stored in CredentialStore
                       // (kept here for in-memory cache + load migration)
    QString type;  // "Local" or "Network"
    // P3 #2: glob-style ignore patterns applied to SyncEngine.
    QStringList ignorePatterns;
};

class ConfigService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString localPath READ localPath WRITE setLocalPath)
    Q_PROPERTY(QString remoteUrl READ remoteUrl WRITE setRemoteUrl)
    Q_PROPERTY(int syncIntervalMinutes READ syncIntervalMinutes WRITE setSyncIntervalMinutes)
    Q_PROPERTY(QString proxyUrl READ proxyUrl WRITE setProxyUrl)
    Q_PROPERTY(int syncRecordRetentionDays READ syncRecordRetentionDays WRITE setSyncRecordRetentionDays)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart)
    Q_PROPERTY(bool minimizeToTray READ minimizeToTray WRITE setMinimizeToTray)
    Q_PROPERTY(bool autoStartMinimize READ autoStartMinimize WRITE setAutoStartMinimize)
    Q_PROPERTY(QString language READ language WRITE setLanguage)
    Q_PROPERTY(QString theme READ theme WRITE setTheme)
    Q_PROPERTY(int fileTransferTimeoutSeconds READ fileTransferTimeoutSeconds WRITE setFileTransferTimeoutSeconds)

    Q_PROPERTY(bool autoSyncEnabled READ autoSyncEnabled WRITE setAutoSyncEnabled)
    Q_PROPERTY(int maxBackgroundRepos READ maxBackgroundRepos WRITE setMaxBackgroundRepos)
    Q_PROPERTY(int backgroundPollingInterval READ backgroundPollingInterval WRITE setBackgroundPollingInterval)

public:
    explicit ConfigService(QObject *parent = nullptr);
    ~ConfigService() = default;

    // 基础读写
    Q_INVOKABLE void load();
    Q_INVOKABLE void saveConfig();

    // 仓库管理
    Q_INVOKABLE void addRepository(const QVariantMap &repo);
    Q_INVOKABLE void removeRepository(const QString &name);
    Q_INVOKABLE QVariantList repositories() const;
    Q_INVOKABLE void setActiveRepository(const QString &name);
    Q_INVOKABLE QString activeRepositoryName() const;
    // Rename a repository in-place. Also keeps m_activeRepoName in sync
    // if the active repo was renamed. Returns true if a row was actually
    // updated (false = no such repo, or new name equals old name).
    Q_INVOKABLE bool updateRepositoryName(const QString &oldName, const QString &newName);
    // Edit a repository's URL in-place (e.g. server moved). Returns true
    // if a row was actually updated.
    Q_INVOKABLE bool updateRepositoryUrl(const QString &name, const QString &newUrl);
    // P3 #2: persist per-repo ignore patterns (glob style).
    Q_INVOKABLE bool setRepositoryIgnorePatterns(const QString &name, const QStringList &patterns);

    // SVN 凭证
    Q_INVOKABLE QString getPassword(const QString &repoName) const;

    // 属性读写
    QString localPath() const { return m_localPath; }
    void setLocalPath(const QString &v) { m_localPath = v; }

    QString remoteUrl() const { return m_remoteUrl; }
    void setRemoteUrl(const QString &v) { m_remoteUrl = v; }

    int syncIntervalMinutes() const { return m_syncIntervalMinutes; }
    void setSyncIntervalMinutes(int v) { m_syncIntervalMinutes = v; }

    QString proxyUrl() const { return m_proxyUrl; }
    void setProxyUrl(const QString &v) { m_proxyUrl = v; }

    int syncRecordRetentionDays() const { return m_syncRecordRetentionDays; }
    void setSyncRecordRetentionDays(int v) { m_syncRecordRetentionDays = v; }

    bool autoStart() const { return m_autoStart; }
    void setAutoStart(bool v) { m_autoStart = v; }

    bool minimizeToTray() const { return m_minimizeToTray; }
    void setMinimizeToTray(bool v) { m_minimizeToTray = v; }

    bool autoStartMinimize() const { return m_autoStartMinimize; }
    void setAutoStartMinimize(bool v) { m_autoStartMinimize = v; }

    QString language() const { return m_language; }
    void setLanguage(const QString &v) { m_language = v; }

    QString theme() const { return m_theme; }
    void setTheme(const QString &v) { m_theme = v; }

    int fileTransferTimeoutSeconds() const { return m_fileTransferTimeoutSeconds; }
    void setFileTransferTimeoutSeconds(int v) { m_fileTransferTimeoutSeconds = v; }

    bool autoSyncEnabled() const { return m_autoSyncEnabled; }
    void setAutoSyncEnabled(bool v) { m_autoSyncEnabled = v; }

    int maxBackgroundRepos() const { return m_maxBackgroundRepos; }
    void setMaxBackgroundRepos(int v) { m_maxBackgroundRepos = v; }

    int backgroundPollingInterval() const { return m_backgroundPollingInterval; }
    void setBackgroundPollingInterval(int v) { m_backgroundPollingInterval = v; }

    // P3 #3: expose load/save for unit tests. Safe to call repeatedly;
    // production code only calls them in ctor / setter paths.
    void loadFromDisk();
    void saveToDisk();
    QString configFilePath() const;

signals:
    void repositoriesChanged();

private:

    // 运行时配置
    QString m_localPath;
    QString m_remoteUrl;
    int m_syncIntervalMinutes = 1;
    QString m_proxyUrl;
    int m_syncRecordRetentionDays = 30;
    bool m_autoStart = true;
    bool m_minimizeToTray = true;
    bool m_autoStartMinimize = true;
    QString m_language = "auto";
    QString m_theme = "system";
    int m_fileTransferTimeoutSeconds = 120;
    bool m_autoSyncEnabled = true;
    int m_maxBackgroundRepos = 3;          // max repos to keep in background monitoring
    int m_backgroundPollingInterval = 10;  // background polling interval (minutes)

    // 仓库列表
    QList<Repository> m_repositories;
    QString m_activeRepoName;
};

#endif // CONFIGSERVICE_H

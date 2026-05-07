#ifndef CONFIGSERVICE_H
#define CONFIGSERVICE_H

#include <QObject>
#include <QString>
#include <QVariantList>

struct Repository {
    QString name;
    QString url;
    QString localPath;
    QString username;
    QString password;
    QString type;  // "Local" or "Network"
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

signals:
    void repositoriesChanged();

private:
    void loadFromDisk();
    void saveToDisk();

    QString configFilePath() const;

    // 运行时配置
    QString m_localPath;
    QString m_remoteUrl;
    int m_syncIntervalMinutes = 1;
    QString m_proxyUrl;
    int m_syncRecordRetentionDays = 30;
    bool m_autoStart = true;
    bool m_minimizeToTray = true;

    // 仓库列表
    QList<Repository> m_repositories;
    QString m_activeRepoName;
};

#endif // CONFIGSERVICE_H

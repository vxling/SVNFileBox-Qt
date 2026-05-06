#ifndef CONFIGSERVICE_H
#define CONFIGSERVICE_H

#include <QObject>
#include <QVariantMap>

class ConfigService : public QObject
{
    Q_OBJECT

public:
    explicit ConfigService(QObject *parent = nullptr);
    ~ConfigService() = default;

    Q_INVOKABLE QVariantMap loadConfig() const;
    Q_INVOKABLE void saveConfig(const QVariantMap &config);
    Q_INVOKABLE QString localPath() const;
    Q_INVOKABLE QString remoteUrl() const;
    Q_INVOKABLE QString username() const;

signals:
    void configChanged();

private:
    QString m_configPath;
};

#endif // CONFIGSERVICE_H

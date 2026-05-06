#include "configservice.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>

ConfigService::ConfigService(QObject *parent)
    : QObject(parent)
    , m_configPath(QCoreApplication::applicationDirPath() + "/config.json")
{}

QVariantMap ConfigService::loadConfig() const
{
    QVariantMap map;
    QFile f(m_configPath);
    if (f.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        map = doc.object().toVariantMap();
        f.close();
    }
    return map;
}

void ConfigService::saveConfig(const QVariantMap &config)
{
    QFile f(m_configPath);
    if (f.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(QJsonObject::fromVariantMap(config));
        f.write(doc.toJson());
        f.close();
        emit const_cast<ConfigService*>(this)->configChanged();
    }
}

QString ConfigService::localPath() const { return loadConfig().value("localPath").toString(); }
QString ConfigService::remoteUrl() const { return loadConfig().value("remoteUrl").toString(); }
QString ConfigService::username() const { return loadConfig().value("username").toString(); }

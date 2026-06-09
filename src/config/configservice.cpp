#include "configservice.h"
#include "credentialstore.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

ConfigService::ConfigService(QObject *parent) : QObject(parent)
{
    load();
}

QString ConfigService::configFilePath() const
{
    return QDir(QCoreApplication::applicationDirPath() + "/..")
        .absoluteFilePath(".svnfilebox/config.json");
}

QString ConfigService::getPassword(const QString &repoName) const
{
    for (const auto &r : m_repositories) {
        if (r.name == repoName) return r.password;
    }
    return QString();
}

void ConfigService::load()
{
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        // 默认路径
        m_localPath = QDir::home().absoluteFilePath(".svnfilebox/workcopies");
        return;
    }
    QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    file.close();

    m_localPath = root["localPath"].toString(QDir::home().absoluteFilePath(".svnfilebox/workcopies"));
    m_remoteUrl = root["remoteUrl"].toString();
    m_syncIntervalMinutes = root["syncIntervalMinutes"].toInt(1);
    m_proxyUrl = root["proxyUrl"].toString();
    m_syncRecordRetentionDays = root["syncRecordRetentionDays"].toInt(30);
    m_autoStart = root["autoStart"].toBool(true);
    m_minimizeToTray = root["minimizeToTray"].toBool(true);
    m_autoStartMinimize = root["autoStartMinimize"].toBool(true);
    m_language = root["language"].toString("auto");
    m_theme = root["theme"].toString("system");
    m_fileTransferTimeoutSeconds = root["fileTransferTimeoutSeconds"].toInt(120);
    m_autoSyncEnabled = root["autoSyncEnabled"].toBool(true);
    m_maxBackgroundRepos = root["maxBackgroundRepos"].toInt(3);
    m_backgroundPollingInterval = root["backgroundPollingInterval"].toInt(10);

    m_repositories.clear();
    QJsonArray repos = root["repositories"].toArray();
    for (const QJsonValue &v : repos) {
        QJsonObject o = v.toObject();
        Repository r;
        r.name = o["name"].toString();
        r.url = o["url"].toString();
        r.localPath = o["localPath"].toString();
        r.username = o["username"].toString();
        // Password: try CredentialStore first, fall back to legacy plaintext field.
        auto creds = CredentialStore::retrieve(r.name);
        if (!creds.second.isEmpty()) {
            r.password = creds.second; // decrypt from CredentialStore
        } else {
            r.password = o["password"].toString(); // legacy plaintext fallback
        }
        r.type = o["type"].toString("Local");
        // P3 #2: optional ignore patterns. Older config files won't have
        // this key, so default to empty list.
        QJsonValue igVal = o["ignorePatterns"];
        if (igVal.isArray()) {
            for (const QJsonValue &p : igVal.toArray()) {
                r.ignorePatterns.append(p.toString());
            }
        }
        m_repositories.append(r);
    }
    m_activeRepoName = root["activeRepositoryName"].toString();
}

void ConfigService::saveConfig()
{
    saveToDisk();
    emit repositoriesChanged();
}

void ConfigService::saveToDisk()
{
    QDir().mkpath(QFileInfo(configFilePath()).absolutePath());
    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly)) return;

    QJsonObject root;
    root["localPath"] = m_localPath;
    root["remoteUrl"] = m_remoteUrl;
    root["syncIntervalMinutes"] = m_syncIntervalMinutes;
    root["proxyUrl"] = m_proxyUrl;
    root["syncRecordRetentionDays"] = m_syncRecordRetentionDays;
    root["autoStart"] = m_autoStart;
    root["minimizeToTray"] = m_minimizeToTray;
    root["autoStartMinimize"] = m_autoStartMinimize;
    root["language"] = m_language;
    root["theme"] = m_theme;
    root["fileTransferTimeoutSeconds"] = m_fileTransferTimeoutSeconds;
    root["autoSyncEnabled"] = m_autoSyncEnabled;
    root["maxBackgroundRepos"] = m_maxBackgroundRepos;
    root["backgroundPollingInterval"] = m_backgroundPollingInterval;
    root["activeRepositoryName"] = m_activeRepoName;

    QJsonArray repos;
    for (const Repository &r : m_repositories) {
        QJsonObject o;
        o["name"] = r.name;
        o["url"] = r.url;
        o["localPath"] = r.localPath;
        o["username"] = r.username;
        // Password stored via CredentialStore (machine-bound encryption)
        // to prevent config.json theft across machines.
        o["type"] = r.type;
        // P3 #2: persist ignore patterns
        if (!r.ignorePatterns.isEmpty()) {
            QJsonArray ig;
            for (const QString &p : r.ignorePatterns) ig.append(p);
            o["ignorePatterns"] = ig;
        }
        repos.append(o);
    }
    root["repositories"] = repos;

    file.write(QJsonDocument(root).toJson());
    file.close();
}

void ConfigService::addRepository(const QVariantMap &repo)
{
    Repository r;
    r.name = repo["name"].toString();
    r.url = repo["url"].toString();
    r.localPath = repo["localPath"].toString();
    r.username = repo["username"].toString();
    r.password = repo["password"].toString();
    r.type = repo["type"].canConvert<QString>() ? repo["type"].toString() : QString("Local");
    // P3 #2: ignore patterns from QML/QVariant
    if (repo.contains("ignorePatterns") && repo["ignorePatterns"].canConvert<QVariantList>()) {
        r.ignorePatterns = repo["ignorePatterns"].toStringList();
    }
    m_repositories.append(r);
    // Store credentials with machine-bound encryption, keyed by repoUrl for
    // network repos (local repos have empty url and don't need remote creds).
    CredentialStore::store(r.name, r.username, r.password);
    saveToDisk();
    emit repositoriesChanged();
}

void ConfigService::removeRepository(const QString &name)
{
    // Find repoUrl before erasing so we can remove the CredentialStore entry
    QString repoUrl;
    for (const Repository &r : m_repositories) {
        if (r.name == name) {
            repoUrl = r.url;
            break;
        }
    }
    m_repositories.erase(
        std::remove_if(m_repositories.begin(), m_repositories.end(),
                      [&](const Repository &r) { return r.name == name; }),
        m_repositories.end());
    CredentialStore::remove(name);
    saveToDisk();
    emit repositoriesChanged();
}

QVariantList ConfigService::repositories() const
{
    QVariantList list;
    for (const Repository &r : m_repositories) {
        QVariantMap map;
        map["name"] = r.name;
        map["url"] = r.url;
        map["path"] = r.localPath;
        map["username"] = r.username;
        map["password"] = r.password;
        map["type"] = r.type;
        map["isSelected"] = (r.name == m_activeRepoName);
        // P3 #2: expose ignore patterns to QML for sidebar
        map["ignorePatterns"] = r.ignorePatterns;
        list.append(map);
    }
    return list;
}

void ConfigService::setActiveRepository(const QString &name)
{
    m_activeRepoName = name;
    saveToDisk();
}

QString ConfigService::activeRepositoryName() const
{
    return m_activeRepoName;
}

bool ConfigService::updateRepositoryName(const QString &oldName, const QString &newName)
{
    if (oldName == newName) return false;
    const QString trimmedNew = newName.trimmed();
    if (trimmedNew.isEmpty()) return false;

    for (Repository &r : m_repositories) {
        if (r.name == oldName) {
            r.name = trimmedNew;
            // Keep active-repo pointer consistent
            if (m_activeRepoName == oldName) {
                m_activeRepoName = trimmedNew;
            }
            // CredentialStore is keyed by repoUrl — renaming doesn't affect it
            saveToDisk();
            emit repositoriesChanged();
            return true;
        }
    }
    return false;
}

bool ConfigService::updateRepositoryUrl(const QString &name, const QString &newUrl)
{
    for (Repository &r : m_repositories) {
        if (r.name == name) {
            if (r.url == newUrl) return false;
            r.url = newUrl;
            saveToDisk();
            emit repositoriesChanged();
            return true;
        }
    }
    return false;
}

// P3 #2: set ignore patterns for a repo. Mirrors updateRepositoryUrl pattern.
bool ConfigService::setRepositoryIgnorePatterns(const QString &name, const QStringList &patterns)
{
    for (Repository &r : m_repositories) {
        if (r.name == name) {
            if (r.ignorePatterns == patterns) return false;
            r.ignorePatterns = patterns;
            saveToDisk();
            emit repositoriesChanged();
            return true;
        }
    }
    return false;
}

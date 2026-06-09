#include "repoglobalmanager.h"
#include "repomanager.h"
#include "svn/svnclient.h"
#include "sync/syncengine.h"
#include "sync/commitqueue.h"
#include "svn/svncommandexecutor.h"
#include "config/configservice.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>

namespace SVNFileBox {

RepoGlobalManager::RepoGlobalManager(QObject *parent)
    : QObject(parent)
{
}

RepoGlobalManager::~RepoGlobalManager() = default;

void RepoGlobalManager::createNetworkRepoAsync(const QString &name, const QString &path,
                                                const QString &url, const QString &username,
                                                const QString &password)
{
    // Verify URL with a temporary SVNClient before creating manager
    SVNClient tmpClient;
    tmpClient.setUsername(username);
    tmpClient.setPassword(password);
    if (!tmpClient.testConnection(url)) {
        qWarning() << "[RepoGlobalManager] Cannot connect to URL:" << url;
        return;
    }

    Repository repo{name, path, url, username, password, QStringLiteral("Network"), false};
    auto *manager = new RepoManager(repo, this);
    m_managers.append(manager);
    switchToAsync(manager);
}

void RepoGlobalManager::createLocalRepo(const QString &name, const QString &path,
                                         const QString &url, const QString &username,
                                         const QString &password)
{
    Repository repo{name, path, url, username, password, QStringLiteral("Local"), false};
    auto *manager = new RepoManager(repo, this);
    m_managers.append(manager);
    switchToAsync(manager);
}

void RepoGlobalManager::switchToAsync(RepoManager *newManager)
{
    if (!newManager || m_isDisposed) return;


    // Demote currently active manager to background monitoring (LRU pool)
    if (m_activeManager && m_activeManager != newManager) {
        m_activeManager->background();
        m_backgroundRepos.removeAll(m_activeManager);
        m_backgroundRepos.prepend(m_activeManager);  // most recent at front

        // Evict oldest if pool exceeds limit
        while (m_backgroundRepos.size() > m_maxBackgroundRepos) {
            RepoManager *oldest = m_backgroundRepos.takeLast();
            oldest->shutdown();
            m_managers.removeAll(oldest);
            oldest->deleteLater();
            qDebug() << "[RepoGlobalManager] Evicted background repo (LRU):"
                     << oldest->repository.name;
        }
        unbindManagerEvents(m_activeManager);
    }

    m_activeManager = newManager;

    // Remove new manager from background pool if it was there
    m_backgroundRepos.removeAll(newManager);

    // Bind new manager signals
    bindManagerEvents(newManager);

    // Focus the new manager (bring to foreground)
    newManager->focus();

    emit activeExecutorChanged(newManager);
    emit filesChanged();
}

void RepoGlobalManager::remove(RepoManager *manager)
{
    if (!manager) return;

    if (m_activeManager == manager) {
        unbindManagerEvents(m_activeManager);
        m_activeManager = nullptr;
    }

    m_backgroundRepos.removeAll(manager);
    manager->shutdown();
    m_managers.removeAll(manager);
    manager->deleteLater();
}

void RepoGlobalManager::shutdownAll()
{
    m_isDisposed = true;
    for (auto *manager : m_managers) {
        manager->shutdown();
    }
    m_backgroundRepos.clear();
}

void RepoGlobalManager::restoreFromConfig(const QVariantList &repoList)
{
    for (const QVariant &v : repoList) {
        QVariantMap map = v.toMap();
        Repository repo{
            map.value(QStringLiteral("name")).toString(),
            map.value(QStringLiteral("path")).toString(),
            map.value(QStringLiteral("url")).toString(),
            map.value(QStringLiteral("username")).toString(),
            map.value(QStringLiteral("password")).toString(),
            map.value(QStringLiteral("type")).toString(),
            false
        };
        if (repo.isValid()) {
            auto *manager = new RepoManager(repo, this);
            m_managers.append(manager);
            // Persistence of rename/edit events is handled by
            // bindManagerEvents() → onManagerRepositoryChanged() using
            // the injected m_configService pointer.
        }
    }
}

void RepoGlobalManager::restoreAndSwitchToLastActive(const QVariantList &repoList,
                                                      const QString &lastActiveName)
{
    if (m_configService) {
        m_maxBackgroundRepos = m_configService->maxBackgroundRepos();
    }
    restoreFromConfig(repoList);

    RepoManager *target = nullptr;
    if (!lastActiveName.isEmpty()) {
        for (auto *mgr : m_managers) {
            if (mgr->repository.name == lastActiveName) {
                target = mgr;
                break;
            }
        }
    }

    if (!target && !m_managers.isEmpty()) {
        target = m_managers.first();
    }

    if (target) {
        switchToAsync(target);
    }
}

void RepoGlobalManager::renameRepo(RepoManager *manager, const QString &newName)
{
    if (!manager) return;
    manager->renameRepo(newName);
    // Persistence + signal forwarding is handled in onManagerRepositoryChanged
    // which is wired up via bindManagerEvents().
}

void RepoGlobalManager::updateRepoUrl(RepoManager *manager, const QString &newUrl)
{
    if (!manager) return;
    manager->updateUrl(newUrl);
    // Persistence + signal forwarding is handled in onManagerRepositoryChanged.
}

void RepoGlobalManager::renameRepoByName(const QString &oldName, const QString &newName)
{
    for (auto *mgr : m_managers) {
        if (mgr->repository.name == oldName) {
            renameRepo(mgr, newName);
            return;
        }
    }
    qWarning() << "[RepoGlobalManager] renameRepoByName: no manager found for" << oldName;
}

void RepoGlobalManager::updateRepoUrlByName(const QString &name, const QString &newUrl)
{
    for (auto *mgr : m_managers) {
        if (mgr->repository.name == name) {
            updateRepoUrl(mgr, newUrl);
            return;
        }
    }
    qWarning() << "[RepoGlobalManager] updateRepoUrlByName: no manager found for" << name;
}

void RepoGlobalManager::bindManagerEvents(RepoManager *manager)
{
    connect(manager, &RepoManager::filesChanged, this, &RepoGlobalManager::onManagerFilesChanged);
    connect(manager, &RepoManager::syncNotification, this, &RepoGlobalManager::onManagerSyncNotification);
    connect(manager, &RepoManager::conflictDetected, this, &RepoGlobalManager::onManagerConflictDetected);
    connect(manager, &RepoManager::credentialExpired, this, &RepoGlobalManager::onManagerCredentialExpired);
    connect(manager, &RepoManager::repositoryChanged, this, &RepoGlobalManager::onManagerRepositoryChanged);
}

void RepoGlobalManager::unbindManagerEvents(RepoManager *manager)
{
    disconnect(manager, &RepoManager::filesChanged, this, &RepoGlobalManager::onManagerFilesChanged);
    disconnect(manager, &RepoManager::syncNotification, this, &RepoGlobalManager::onManagerSyncNotification);
    disconnect(manager, &RepoManager::conflictDetected, this, &RepoGlobalManager::onManagerConflictDetected);
    disconnect(manager, &RepoManager::credentialExpired, this, &RepoGlobalManager::onManagerCredentialExpired);
    disconnect(manager, &RepoManager::repositoryChanged, this, &RepoGlobalManager::onManagerRepositoryChanged);
}

void RepoGlobalManager::connectActiveRepoSignals(QObject *receiver)
{
    if (!receiver || !m_activeManager) return;

    // Forward all active manager signals to the receiver
    // receiver is QObject* (QML) — use string-based to connect to dynamic signals
    connect(m_activeManager, SIGNAL(filesChanged()), receiver, SLOT(filesChanged()), Qt::UniqueConnection);
    connect(m_activeManager, SIGNAL(syncNotification(QString)), receiver, SLOT(syncNotification(QString)), Qt::UniqueConnection);
    connect(m_activeManager, SIGNAL(conflictDetected(QStringList)), receiver, SLOT(conflictDetected(QStringList)), Qt::UniqueConnection);
    connect(m_activeManager, SIGNAL(credentialExpired(QString,QString)), receiver, SLOT(credentialExpired(QString,QString)), Qt::UniqueConnection);
    connect(m_activeManager, SIGNAL(repositoryChanged(QString,QString,QString,QString)),
            receiver, SLOT(repositoryChanged(QString,QString,QString,QString)), Qt::UniqueConnection);
}

void RepoGlobalManager::onManagerFilesChanged()
{
    emit filesChanged();
}

void RepoGlobalManager::onManagerSyncNotification(const QString &msg)
{
    emit syncNotification(msg);
}

void RepoGlobalManager::onManagerConflictDetected(const QStringList &files)
{
    emit conflictDetected(files);
}

void RepoGlobalManager::onManagerCredentialExpired(const QString &repoName, const QString &path)
{
    emit credentialExpired(repoName, path);
}

void RepoGlobalManager::onManagerRepositoryChanged(const QString &oldName,
                                                    const QString &newName,
                                                    const QString &oldUrl,
                                                    const QString &newUrl)
{
    qDebug() << "[RepoGlobalManager] repositoryChanged:" << oldName
             << "->" << newName << "| url:" << oldUrl << "->" << newUrl;

    // 1) Persist to config.json
    if (m_configService) {
        if (oldName != newName) {
            m_configService->updateRepositoryName(oldName, newName);
        }
        if (oldUrl != newUrl) {
            m_configService->updateRepositoryUrl(newName, newUrl);
        }
    } else {
        qWarning() << "[RepoGlobalManager] no ConfigService injected;"
                   << "rename/url edit not persisted to disk";
    }

    // 2) Forward to QML
    emit repositoryChanged(oldName, newName, oldUrl, newUrl);
}

void RepoGlobalManager::enqueueCommit(const QString &path, int operation, const QString &fromPath)
{
    if (m_activeManager) {
        m_activeManager->enqueueCommit(path, operation, fromPath);
    }
}

} // namespace SVNFileBox
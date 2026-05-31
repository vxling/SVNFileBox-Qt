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
    if (!tmpClient.testConnection(url, username, password)) {
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

    // Dismiss currently active manager
    if (m_activeManager && m_activeManager != newManager) {
        m_activeManager->dismiss();
    }

    // Unbind old signals
    if (m_activeManager) {
        unbindManagerEvents(m_activeManager);
    }

    m_activeManager = newManager;

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
        m_activeManager->dismiss();
        unbindManagerEvents(m_activeManager);
        m_activeManager = nullptr;
    }

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
        }
    }
}

void RepoGlobalManager::restoreAndSwitchToLastActive(const QVariantList &repoList,
                                                      const QString &lastActiveName)
{
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

void RepoGlobalManager::bindManagerEvents(RepoManager *manager)
{
    connect(manager, &RepoManager::filesChanged, this, &RepoGlobalManager::onManagerFilesChanged);
    connect(manager, &RepoManager::syncNotification, this, &RepoGlobalManager::onManagerSyncNotification);
    connect(manager, &RepoManager::conflictDetected, this, &RepoGlobalManager::onManagerConflictDetected);
}

void RepoGlobalManager::unbindManagerEvents(RepoManager *manager)
{
    disconnect(manager, &RepoManager::filesChanged, this, &RepoGlobalManager::onManagerFilesChanged);
    disconnect(manager, &RepoManager::syncNotification, this, &RepoGlobalManager::onManagerSyncNotification);
    disconnect(manager, &RepoManager::conflictDetected, this, &RepoGlobalManager::onManagerConflictDetected);
}

void RepoGlobalManager::connectActiveRepoSignals(QObject *receiver)
{
    if (!receiver || !m_activeManager) return;

    // Forward all active manager signals to the receiver
    // receiver is QObject* (QML) — use string-based to connect to dynamic signals
    connect(m_activeManager, SIGNAL(filesChanged()), receiver, SLOT(filesChanged()), Qt::UniqueConnection);
    connect(m_activeManager, SIGNAL(syncNotification(QString)), receiver, SLOT(syncNotification(QString)), Qt::UniqueConnection);
    connect(m_activeManager, SIGNAL(conflictDetected(QStringList)), receiver, SLOT(conflictDetected(QStringList)), Qt::UniqueConnection);
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

} // namespace SVNFileBox
#include "repomanager.h"
#include "svn/svncommandexecutor.h"
#include "svn/svncommand.h"
#include "sync/syncengine.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>

namespace SVNFileBox {

RepoManager::RepoManager(const Repository &repo, QObject *parent)
    : QObject(parent), repository(repo)
{
    executor = new SvnCommandExecutor(nullptr, this);
    syncEngine = new SyncEngine(this);

    connect(executor, &SvnCommandExecutor::onCommandCompleted, this, [this](const SvnCommandResult &) {
        emitFilesChanged();
    });
    connect(syncEngine, &SyncEngine::syncNotification, this, &RepoManager::emitSyncNotification);
    connect(syncEngine, &SyncEngine::conflictDetected, this, &RepoManager::emitConflictDetected);
}

RepoManager::~RepoManager() = default;

void RepoManager::focus()
{
    if (m_state == RepoState::Focused) return;
    qDebug() << "[RepoManager] focus:" << repository.name;

    m_state = RepoState::Focused;
    syncEngine->watchPath(repository.path);
    syncEngine->startSync(repository.name, repository.path, repository.url,
                          repository.username, repository.password);
    emit stateChanged(m_state);
    emitFilesChanged();
}

void RepoManager::dismiss()
{
    if (m_state == RepoState::None) return;
    qDebug() << "[RepoManager] dismiss:" << repository.name;

    m_state = RepoState::Dismissed;
    syncEngine->stopSync();
    emit stateChanged(m_state);
}

void RepoManager::shutdown()
{
    qDebug() << "[RepoManager] shutdown:" << repository.name;
    m_state = RepoState::None;
    syncEngine->stopSync();
}

void RepoManager::emitFilesChanged()
{
    emit filesChanged();
}

void RepoManager::emitSyncNotification(const QString &msg)
{
    emit syncNotification(msg);
}

void RepoManager::emitConflictDetected(const QStringList &files)
{
    emit conflictDetected(files);
}

} // namespace SVNFileBox
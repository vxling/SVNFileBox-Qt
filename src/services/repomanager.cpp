#include "repomanager.h"
#include "svn/svnclient.h"
#include "svn/svncommandexecutor.h"
#include "svn/svncommand.h"
#include "sync/syncengine.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>

namespace SVNFileBox {

RepoManager::RepoManager(const Repository &repo, QObject *parent)
    : QObject(parent), repository(repo)
{
    // Each repo gets its own SVNClient for thread-safe write operations.
    // The global svnClient (main.cpp) handles shared read operations like
    // checkout and repo info. Per-repo clients handle local writes via
    // executor and sync operations.
    SVNClient *repoClient = new SVNClient(this);
    executor = new SvnCommandExecutor(this);
    syncEngine = new SyncEngine(this);
    syncEngine->setSvnClient(repoClient);
    m_commitQueue = new CommitQueue(this);
    syncEngine->setCommitQueue(m_commitQueue);

    connect(executor, &SvnCommandExecutor::onCommandCompleted, this, [this](const SvnCommandResult &) {
        emitFilesChanged();
    });
    // Auth error: SVN auth permanently failed even after retry. Bubble up.
    connect(executor, &SvnCommandExecutor::onAuthError, this,
            [this](const QString &path) {
        qWarning() << "[RepoManager] Credential expired for" << repository.name
                   << "at" << path;
        emit credentialExpired(repository.name, path);
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
    syncEngine->setIgnorePatterns(repository.ignorePatterns);
    syncEngine->setBackgroundMode(false);
    syncEngine->watchPath(repository.path);
    syncEngine->startSync(repository.name, repository.path, repository.url,
                          repository.username, repository.password);
    emit stateChanged(m_state);
    emitFilesChanged();
}

void RepoManager::background()
{
    if (m_state == RepoState::Background) return;
    if (m_state == RepoState::None || m_state == RepoState::Closed) return;
    qDebug() << "[RepoManager] background:" << repository.name;

    m_state = RepoState::Background;
    syncEngine->setBackgroundMode(true);
    emit stateChanged(m_state);
}

void RepoManager::dismiss()
{
    // dismiss() — backward-compatible alias for background()
    background();
}

void RepoManager::shutdown()
{
    qDebug() << "[RepoManager] shutdown:" << repository.name;
    m_state = RepoState::Closed;
    syncEngine->stopSync();
    if (executor) {
        executor->stop();
        executor->waitForDrained(35000);
    }
}

void RepoManager::renameRepo(const QString &newName)
{
    const QString oldName = repository.name;
    if (!repository.setName(newName)) {
        qDebug() << "[RepoManager] renameRepo: no change for" << oldName;
        return;
    }
    qDebug() << "[RepoManager] renameRepo:" << oldName << "->" << repository.name;
    // P3 review fix (M5): the per-repo SyncEngine cached the repo name at
    // startSync() time. If we don't update it, all subsequent sync records
    // and notifications will be logged under the OLD name, even though
    // every other surface (sidebar, config.json, signals) uses the NEW
    // name. setRepoName() takes the S2 mutex so it's safe to call from
    // the main thread while the worker is mid-cycle.
    if (syncEngine) {
        syncEngine->setRepoName(repository.name);
    }
    emit repositoryChanged(oldName, repository.name, repository.url, repository.url);
    // State doesn't change here, but emit stateChanged-equivalent: callers
    // re-render on repositoryChanged. We deliberately do NOT call
    // syncEngine->startSync again — the working-copy path is unchanged.
}

void RepoManager::updateUrl(const QString &newUrl)
{
    const QString oldUrl = repository.url;
    if (!repository.setUrl(newUrl)) {
        qDebug() << "[RepoManager] updateUrl: no change";
        return;
    }
    qDebug() << "[RepoManager] updateUrl:" << oldUrl << "->" << repository.url;
    emit repositoryChanged(repository.name, repository.name, oldUrl, repository.url);
}

void RepoManager::emitFilesChanged()
{
    emit filesChanged();
}

void RepoManager::emitSyncNotification(const QString &msg)
{
    if (m_state == RepoState::Background && isRoutineNotification(msg))
        return;  // suppress routine notifications in background mode
    emit syncNotification(msg);
}

// Routine notifications are suppressed when repo is in background monitoring mode.
// They indicate normal sync activity that doesn't need user attention.
bool RepoManager::isRoutineNotification(const QString &msg) const
{
    // clang-format off
    static const QStringView routinePrefixes[] = {
        QStringLiteral(u"批量同步完成"),
        QStringLiteral(u"已同步删除: "),
        QStringLiteral(u"已同步: "),
        QStringLiteral(u"同步失败: "),
        QStringLiteral(u"同步已启动: "),
    };
    // clang-format on
    for (const QStringView &p : routinePrefixes) {
        if (msg.startsWith(p))
            return true;
    }
    return false;
}

void RepoManager::emitConflictDetected(const QStringList &files)
{
    emit conflictDetected(files);
}

void RepoManager::enqueueCommit(const QString &path, int operation, const QString &fromPath)
{
    if (m_commitQueue) {
        m_commitQueue->enqueue(path, operation, fromPath);
    }
}

} // namespace SVNFileBox
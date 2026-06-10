#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtCore/QVariantMap>
#include "svn/svncommand.h"
#include "svn/svncommandexecutor.h"
#include "sync/syncengine.h"
#include "sync/commitqueue.h"

namespace SVNFileBox {

struct Repository {
    QString name;
    QString path;
    QString url;
    QString username;
    QString password;
    QString type; // "Local" or "Network"
    bool isActive = false;
    // P3 #2: glob-style ignore patterns applied to SyncEngine.
    QStringList ignorePatterns;

    bool isValid() const { return !name.isEmpty() && !path.isEmpty(); }

    // In-place mutation helpers. Mirrors WPF Repository field updates from
    // EditRepoWindow. Return true on actual change so callers can decide
    // whether to persist and emit signals.
    bool setName(const QString &newName) {
        if (newName.trimmed().isEmpty() || newName == name) return false;
        name = newName.trimmed();
        return true;
    }
    bool setUrl(const QString &newUrl) {
        if (newUrl == url) return false;
        url = newUrl;
        return true;
    }
};

enum class RepoState {
    None,       // Initial state, no sync
    Focused,    // Active, full notifications, normal polling
    Background, // Background monitoring, suppressed routine notifications, longer polling
    Closed,     // Sync stopped, resources released
};

class RepoManager : public QObject
{
    Q_OBJECT

public:
    explicit RepoManager(const Repository &repo, QObject *parent = nullptr);
    ~RepoManager() override;

    Repository repository;
    SvnCommandExecutor *executor = nullptr;
    SyncEngine *syncEngine = nullptr;

    RepoState state() const { return m_state; }

    Q_INVOKABLE void focus();
    Q_INVOKABLE void background();  // demote to background monitoring
    Q_INVOKABLE void dismiss();     // alias for background (backward compat)
    Q_INVOKABLE void shutdown();    // stop and release all resources
    Q_INVOKABLE void renameRepo(const QString &newName);
    Q_INVOKABLE void updateUrl(const QString &newUrl);
    Q_INVOKABLE SvnCommandExecutor *activeExecutor() const { return executor; }
    Q_INVOKABLE CommitQueue *commitQueue() const { return m_commitQueue; }
    Q_INVOKABLE void enqueueCommit(const QString &path, int operation, const QString &fromPath = QString());

    void emitFilesChanged();
    void emitSyncNotification(const QString &msg);
    bool isRoutineNotification(const QString &msg) const;
    void emitConflictDetected(const QStringList &files);

signals:
    void filesChanged();
    void repositoryFocused(const QString &path);  // emitted when repo comes to foreground
    void syncNotification(const QString &message);
    void conflictDetected(const QStringList &conflictedFiles);
    void stateChanged(RepoState newState);
    // Repository identity (name/url/path) changed at runtime via rename/edit.
    // QML sidebar bindings should re-read repository.name / .url; the model
    // row is updated in-place by RepoGlobalManager::onRepositoryChanged.
    void repositoryChanged(const QString &oldName,
                           const QString &newName,
                           const QString &oldUrl,
                           const QString &newUrl);
    // Credentials expired or invalid for this repo. Mirrors WPF's
    // CredentialExpired event. Listeners (RepoGlobalManager, QML) should
    // show a "update credentials" dialog and call svnClient->clearAuthCache()
    // to force re-prompt.
    void credentialExpired(const QString &repoName, const QString &path);

private:
    RepoState m_state = RepoState::None;
    CommitQueue *m_commitQueue = nullptr;
};

} // namespace SVNFileBox
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
    None,
    Focused,
    Dismissed,
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
    Q_INVOKABLE void dismiss();
    Q_INVOKABLE void shutdown();
    Q_INVOKABLE void renameRepo(const QString &newName);
    Q_INVOKABLE void updateUrl(const QString &newUrl);
    Q_INVOKABLE SvnCommandExecutor *activeExecutor() const { return executor; }

    void emitFilesChanged();
    void emitSyncNotification(const QString &msg);
    void emitConflictDetected(const QStringList &files);

signals:
    void filesChanged();
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
};

} // namespace SVNFileBox
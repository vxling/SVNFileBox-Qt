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
    Q_INVOKABLE SvnCommandExecutor *activeExecutor() const { return executor; }

    void emitFilesChanged();
    void emitSyncNotification(const QString &msg);
    void emitConflictDetected(const QStringList &files);

signals:
    void filesChanged();
    void syncNotification(const QString &message);
    void conflictDetected(const QStringList &conflictedFiles);
    void stateChanged(RepoState newState);
    // Credentials expired or invalid for this repo. Mirrors WPF's
    // CredentialExpired event. Listeners (RepoGlobalManager, QML) should
    // show a "update credentials" dialog and call svnClient->clearAuthCache()
    // to force re-prompt.
    void credentialExpired(const QString &repoName, const QString &path);

private:
    RepoState m_state = RepoState::None;
};

} // namespace SVNFileBox
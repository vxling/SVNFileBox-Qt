#pragma once
#include "repomanager.h"
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariantList>

namespace SVNFileBox {

class RepoGlobalManager : public QObject
{
    Q_OBJECT

public:
    explicit RepoGlobalManager(QObject *parent = nullptr);
    ~RepoGlobalManager() override;

    QList<RepoManager *> managers() const { return m_managers; }
    RepoManager *activeManager() const { return m_activeManager; }
    bool isEmpty() const { return m_managers.isEmpty(); }

    Q_INVOKABLE void connectActiveRepoSignals(QObject *receiver);

signals:
    void filesChanged();
    void syncNotification(const QString &message);
    void conflictDetected(const QStringList &conflictedFiles);
    void activeExecutorChanged(RepoManager *manager);
    // Forwarded credential-expired event from any RepoManager
    void credentialExpired(const QString &repoName, const QString &path);

public slots:
    void createNetworkRepoAsync(const QString &name, const QString &path,
                                 const QString &url, const QString &username,
                                 const QString &password);
    void createLocalRepo(const QString &name, const QString &path,
                          const QString &url, const QString &username,
                          const QString &password);
    Q_INVOKABLE void switchToAsync(RepoManager *newManager);
    Q_INVOKABLE void remove(RepoManager *manager);
    Q_INVOKABLE void shutdownAll();
    Q_INVOKABLE void restoreFromConfig(const QVariantList &repoList);
    Q_INVOKABLE void restoreAndSwitchToLastActive(const QVariantList &repoList,
                                                   const QString &lastActiveName);

private slots:
    void onManagerFilesChanged();
    void onManagerSyncNotification(const QString &msg);
    void onManagerConflictDetected(const QStringList &files);
    void onManagerCredentialExpired(const QString &repoName, const QString &path);

private:
    void bindManagerEvents(RepoManager *manager);
    void unbindManagerEvents(RepoManager *manager);

    QList<RepoManager *> m_managers;
    RepoManager *m_activeManager = nullptr;
    bool m_isDisposed = false;
};

} // namespace SVNFileBox
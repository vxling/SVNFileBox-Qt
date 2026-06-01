#pragma once
#include "repomanager.h"
#include "config/configservice.h"
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

    // P3: allow main.cpp to inject the ConfigService used for persistence
    // of rename/url edits. We can't grab it from QML easily, so injection
    // keeps the global manager decoupled.
    void setConfigService(ConfigService *svc) { m_configService = svc; }

    Q_INVOKABLE void connectActiveRepoSignals(QObject *receiver);

signals:
    void filesChanged();
    void syncNotification(const QString &message);
    void conflictDetected(const QStringList &conflictedFiles);
    void activeExecutorChanged(RepoManager *manager);
    // Forwarded credential-expired event from any RepoManager
    void credentialExpired(const QString &repoName, const QString &path);
    // Forwarded rename/url-edit event. QML sidebar listens to update the
    // model row in-place without rebuilding the list.
    void repositoryChanged(const QString &oldName,
                           const QString &newName,
                           const QString &oldUrl,
                           const QString &newUrl);

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
    // P3: rename a live repo manager. Updates ConfigService in place
    // (saves .svnfilebox/config.json synchronously) and emits
    // repositoryChanged so the QML sidebar row can be patched in-place.
    // Pass a non-null ConfigService via main.cpp wiring (TODO: add setter
    // if you want to inject). For now, callers can use the convenience
    // helper renameRepoByName() that resolves manager by name.
    Q_INVOKABLE void renameRepo(RepoManager *manager, const QString &newName);
    Q_INVOKABLE void updateRepoUrl(RepoManager *manager, const QString &newUrl);
    // Convenience: resolve manager by name and rename. No-op if not found.
    Q_INVOKABLE void renameRepoByName(const QString &oldName, const QString &newName);
    // Convenience: resolve manager by name and update URL. No-op if not found.
    Q_INVOKABLE void updateRepoUrlByName(const QString &name, const QString &newUrl);

private slots:
    void onManagerFilesChanged();
    void onManagerSyncNotification(const QString &msg);
    void onManagerConflictDetected(const QStringList &files);
    void onManagerCredentialExpired(const QString &repoName, const QString &path);
    void onManagerRepositoryChanged(const QString &oldName,
                                    const QString &newName,
                                    const QString &oldUrl,
                                    const QString &newUrl);

private:
    void bindManagerEvents(RepoManager *manager);
    void unbindManagerEvents(RepoManager *manager);

    QList<RepoManager *> m_managers;
    RepoManager *m_activeManager = nullptr;
    bool m_isDisposed = false;
    ConfigService *m_configService = nullptr; // injected from main.cpp
};

} // namespace SVNFileBox
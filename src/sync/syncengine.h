#ifndef SYNCENGINE_H
#define SYNCENGINE_H

#include <QObject>
#include <QString>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QSet>
#include <QMutex>
#include <QMutexLocker>

class SVNClient;
class SyncRecordService;
#include "commitqueue.h"

class SyncEngine : public QObject
{
    Q_OBJECT

public:
    explicit SyncEngine(QObject *parent = nullptr);
    ~SyncEngine();

    void setSvnClient(SVNClient *client) { m_svnClient = client; }
    void setSyncRecordService(SyncRecordService *svc) { m_recordService = svc; }

    Q_INVOKABLE void startSync(const QString &repoName, const QString &localPath, const QString &remoteUrl,
                                const QString &username = QString(), const QString &password = QString());
    Q_INVOKABLE void stopSync();
    Q_INVOKABLE void syncNow();
    Q_INVOKABLE void scanAndCommit();
    Q_INVOKABLE void watchPath(const QString &path);
    Q_INVOKABLE QString status() const;
    Q_INVOKABLE QStringList getConflictedFiles() const;
    // Rich conflict info: returns a list of QVariantMaps per file with
    // {path, kind ("text"|"tree"|"property"), localModifiedTime, serverModifiedTime,
    //  selectedResolution, baseRevision, incomingRevision}.
    // Mirrors WPF ConflictedFileInfo. Empty if no conflicts.
    Q_INVOKABLE QVariantList getConflictedFileInfo() const;
    Q_INVOKABLE void resolveConflict(const QString &accept);
    Q_INVOKABLE void resolveConflictForFile(const QString &filePath, const QString &accept);

    void setFileWatcher(QFileSystemWatcher *watcher);
    void DisableFileWatcher();
    void ReEnableFileWatcher();

signals:
    void syncStarted();
    void syncNotification(const QString &message);
    void filesChanged();
    void conflictDetected(const QStringList &files);

private slots:
    void onFileChanged(const QString &path);
    void onDebounceTimer();
    void onPollTimer();
    void onFullSyncTimer();

private:
    void commitFile(const QString &filePath);
    void pollServer();
    void fullScan();
    int handleConflicts();
    void retryPending();
    bool isSvnManaged(const QString &path) const;
    QString parentDir(const QString &filePath) const;
    void addPending(const QString &path);
    bool isTempFile(const QString &path) const;

    SVNClient *m_svnClient = nullptr;
    SyncRecordService *m_recordService = nullptr;

    QString m_repoName;
    QString m_localPath;
    QString m_remoteUrl;
    QString m_username;
    QString m_password;

    QFileSystemWatcher *m_watcher = nullptr;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    QTimer *m_debounceTimer = nullptr;
    QTimer *m_pollTimer = nullptr;
    QTimer *m_fullSyncTimer = nullptr;

    bool m_syncing = false;
    bool m_pausedByConflict = false;
    bool m_scanning = false;
    QMutex m_scanMutex;
    int m_pollIntervalSec = 60;
    int m_staleCounter = 0;
};

#endif // SYNCENGINE_H

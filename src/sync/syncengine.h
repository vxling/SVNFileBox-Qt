#ifndef SYNCENGINE_H
#define SYNCENGINE_H

#include <QObject>
#include <QString>
#include <QFileSystemWatcher>
#include <QTimer>
#include "ignorepattern.h"
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
    // P3 #2: per-repo ignore patterns (glob style, e.g. "*.tmp", "build/",
    // ".DS_Store"). Compared as a QRegularExpression wildcard. Empty = no
    // filtering. Patterns are matched against the full file path relative
    // to m_localPath (basename match for plain names, full path match for
    // patterns containing "/"). Mirrors WPF's SyncService filter at
    // SyncService.cs:284-308.
    void setIgnorePatterns(const QStringList &patterns) { m_ignorePatterns = patterns; m_ignoreRegexes = SVNFileBox::compileIgnorePatterns(patterns); }
    QStringList ignorePatterns() const { return m_ignorePatterns; }
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
    void onDirChanged(const QString &path);
    void onWatcherError(int err);
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
    // Add a directory tree to m_watcher recursively. Mirrors WPF
    // FileSystemWatcher which IS recursive; QFileSystemWatcher is not.
    void addPathRecursive(const QString &rootPath);
    // Try to reconnect the file watcher after failure, with backoff.
    void reconnectWatcher();
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
    // P3 review fix (S2): protect conflict-pause + scanning flags. Without
    // this, status() (called from QML) and onDebounceTimer() (called from
    // QTimer on the main event loop) race on `m_pausedByConflict`, and a
    // quick pause/resume cycle can leave the timer firing once after the
    // pause. The mutex is checked-only: holding it for the entire body of
    // onDebounceTimer() would block status() reads (UI hiccup), so we
    // only lock for the read of the flag (one-line critical section).
    mutable QMutex m_stateMutex;
    int m_pollIntervalSec = 60;
    int m_staleCounter = 0;
    // FileWatcher reconnection: when watcher fails, try to rebuild it
    // with exponential backoff (5s, 10s, then give up).
    int m_watcherRetryCount = 0;
    // P3 #2: ignore patterns
    QStringList m_ignorePatterns;
    QList<QRegularExpression> m_ignoreRegexes;
    // Returns true if any ignore pattern matches the path. Implementation
    // lives in syncengine.cpp; logic itself lives in the standalone
    // ignorepattern.{h,cpp} helper so it can be unit-tested.
    bool isPathIgnored(const QString &path) const;
};

#endif // SYNCENGINE_H

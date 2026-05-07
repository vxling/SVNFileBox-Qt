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
    Q_INVOKABLE QString status() const;

signals:
    void syncStarted();
    void syncNotification(const QString &message);
    void filesChanged();

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

    SVNClient *m_svnClient = nullptr;
    SyncRecordService *m_recordService = nullptr;

    QString m_repoName;
    QString m_localPath;
    QString m_remoteUrl;
    QString m_username;
    QString m_password;

    QFileSystemWatcher *m_watcher = nullptr;
    QTimer *m_debounceTimer = nullptr;
    QTimer *m_pollTimer = nullptr;
    QTimer *m_fullSyncTimer = nullptr;

    QSet<QString> m_pendingFiles;
    QMutex m_pendingMutex;

    bool m_syncing = false;
    int m_pollIntervalSec = 60;
};

#endif // SYNCENGINE_H

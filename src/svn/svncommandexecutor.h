#pragma once
#include "svncommand.h"
#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QQueue>
#include <QtCore/QWaitCondition>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QVariant>
#include <QtCore/QRegularExpression>
#include <QtCore/QSemaphore>
#include <QtCore/QDateTime>

class SVNClient;

namespace SVNFileBox {

class SvnCommandExecutor : public QObject
{
    Q_OBJECT

public:
    explicit SvnCommandExecutor(SVNClient *svnClient, QObject *parent = nullptr);
    ~SvnCommandExecutor() override;

    // ── Public API ────────────────────────────────────────────
    // ReadOnly: blocks caller thread and returns synchronously
    Q_INVOKABLE SvnQueryResult execute(SvnCommand cmd, const QString &path,
                                       const QString &fromPath = QString(),
                                       const QString &message = QString(),
                                       const QString &repoUrl = QString(),
                                       const QString &username = QString(),
                                       const QString &password = QString(),
                                       bool depth = false,
                                       const QString &accept = QString());

    // HeavyWrite: returns immediately, result delivered via onCommandCompleted
    Q_INVOKABLE void executeHeavyWrite(SvnCommand cmd, const QString &path,
                                       const QString &fromPath = QString(),
                                       const QString &message = QString(),
                                       const QString &repoUrl = QString(),
                                       const QString &username = QString(),
                                       const QString &password = QString(),
                                       const QStringList &updatePaths = QStringList(),
                                       const QString &accept = QString());

    // LocalWrite: fire-and-forget, result via onCommandCompleted
    Q_INVOKABLE void executeLocalWrite(SvnCommand cmd, const QString &path,
                                       const QString &fromPath = QString(),
                                       const QString &message = QString(),
                                       const QString &accept = QString());

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool waitForDrained(int timeoutMs = 30000);

signals:
    void onCommandCompleted(const SvnCommandResult &result);
    void onSyncNotification(const QString &message);
    void onSyncError(const QString &error);
    void onTimeout(const QString &op, const QString &path);
    void onAuthError(const QString &path);

private:
    void runWorkerLoop();
    void processItem(const SvnCommandItem &item);
    SvnQueryResult executeReadOnly(SvnCommand cmd, const QString &path,
                                   const QString &fromPath,
                                   const QString &message,
                                   const QString &repoUrl,
                                   const QString &username,
                                   const QString &password,
                                   bool depth);
    bool tryEnqueueLocalWrite(const SvnCommandItem &item);
    bool tryEnqueueHeavyWrite(const SvnCommandItem &item);
    void removeFromDedup(const SvnCommandItem &item);
    QString dedupKey(const SvnCommandItem &item) const;

    // ── Concurrency ────────────────────────────────────────────
    static QSemaphore s_writeSemaphore;   // Serializes all writes (1 permit), initialized in .cpp
    static constexpr int LOCK_WAIT_TIMEOUT_MS = 30'000;

    SVNClient *m_svnClient = nullptr;
    QQueue<SvnCommandItem> m_localWriteQueue;
    QQueue<SvnCommandItem> m_heavyWriteQueue;
    QMutex m_queueMutex;
    QWaitCondition m_queueCond;
    QMap<QString, SvnCommandItem> m_dedup;
    QThread *m_workerThread = nullptr;
    bool m_running = false;
    bool m_drainMode = false;
    bool m_drained = false;
    QMutex m_drainMutex;
    QWaitCondition m_drainCond;
    // Throttle svn cleanup: run at most once per 60s per executor
    QDateTime m_lastCleanupAt;
    void maybeRunStaleLockCleanup(const QString &path);
};

} // namespace SVNFileBox
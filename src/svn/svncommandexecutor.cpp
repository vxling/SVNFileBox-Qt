#include "svncommandexecutor.h"
#include "svnclient.h"
#include "svnclient.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

namespace SVNFileBox {

// Static concurrency primitive — one permit = one write at a time across all instances
QSemaphore SvnCommandExecutor::s_writeSemaphore(1);

SvnCommandExecutor::SvnCommandExecutor(SVNClient *svnClient, QObject *parent)
    : QObject(parent)
    , m_svnClient(svnClient)
{
    Q_ASSERT(m_svnClient != nullptr);
}

SvnCommandExecutor::~SvnCommandExecutor()
{
    stop();
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(5000);
        delete m_workerThread;
    }
}

// ── Public API ──────────────────────────────────────────────────

void SvnCommandExecutor::start()
{
    if (m_workerThread && m_workerThread->isRunning())
        return;

    m_running = true;
    m_drainMode = false;
    m_workerThread = QThread::create([this]() { runWorkerLoop(); });
    m_workerThread->setObjectName("SvnCommandExecutor");
    m_workerThread->start();
    qDebug() << "[SvnCommandExecutor] Started";
}

void SvnCommandExecutor::stop()
{
    // Drain all pending items before stopping (max 30s)
    {
        QMutexLocker drainLocker(&m_drainMutex);
        m_drainMode = true;
        m_drained = false;
        m_queueCond.wakeAll();
        m_drainCond.wait(&m_drainMutex, 30000);
        m_drainMode = false;
        m_drained = false;
    }
    m_running = false;
    m_queueCond.wakeAll();
    qDebug() << "[SvnCommandExecutor] Stop requested (drain complete)";
}

bool SvnCommandExecutor::waitForDrained(int timeoutMs)
{
    QMutexLocker locker(&m_drainMutex);
    if (m_drained)
        return true;
    return m_drainCond.wait(&m_drainMutex, timeoutMs);
}

SvnQueryResult SvnCommandExecutor::execute(
    SvnCommand cmd,
    const QString &path,
    const QString &fromPath,
    const QString &message,
    const QString &repoUrl,
    const QString &username,
    const QString &password,
    bool depth,
    const QString &accept)
{
    auto category = commandCategory(cmd);

    // ReadOnly: execute synchronously on caller thread
    if (category == SvnCommandCategory::ReadOnly) {
        return executeReadOnly(cmd, path, fromPath, message, repoUrl, username, password, depth);
    }

    // LocalWrite: enqueue (fire-and-forget)
    if (category == SvnCommandCategory::LocalWrite) {
        auto item = SvnCommandItem::make(cmd, path, fromPath, QString(), QString(), QString(), QString(), QStringList(), accept);
        if (tryEnqueueLocalWrite(item)) {
            QMutexLocker locker(&m_queueMutex);
            m_localWriteQueue.enqueue(item);
            m_queueCond.wakeOne();
        }
        return SvnQueryResult::Ok(QString());
    }

    // HeavyWrite: enqueue, result via signal
    auto item = SvnCommandItem::make(cmd, path, fromPath, message, repoUrl, username, password, QStringList(), accept);
    if (!tryEnqueueHeavyWrite(item))
        return SvnQueryResult::Fail(QStringLiteral("Deduplicated: skipped"));

    {
        QMutexLocker locker(&m_queueMutex);
        m_heavyWriteQueue.enqueue(item);
        m_queueCond.wakeOne();
    }
    return SvnQueryResult::Ok(QString());
}

void SvnCommandExecutor::executeHeavyWrite(
    SvnCommand cmd,
    const QString &path,
    const QString &fromPath,
    const QString &message,
    const QString &repoUrl,
    const QString &username,
    const QString &password,
    const QStringList &updatePaths,
    const QString &accept)
{
    auto item = SvnCommandItem::make(cmd, path, fromPath, message, repoUrl, username, password, updatePaths, accept);
    if (!tryEnqueueHeavyWrite(item))
        return;
    {
        QMutexLocker locker(&m_queueMutex);
        m_heavyWriteQueue.enqueue(item);
        m_queueCond.wakeOne();
    }
}

void SvnCommandExecutor::executeLocalWrite(
    SvnCommand cmd,
    const QString &path,
    const QString &fromPath,
    const QString &message,
    const QString &accept)
{
    auto item = SvnCommandItem::make(cmd, path, fromPath, message, QString(), QString(), QString(), QStringList(), accept);
    if (tryEnqueueLocalWrite(item)) {
        QMutexLocker locker(&m_queueMutex);
        m_localWriteQueue.enqueue(item);
        m_queueCond.wakeOne();
    }
}

// ── Worker loop ─────────────────────────────────────────────────

void SvnCommandExecutor::runWorkerLoop()
{
    while (m_running || m_drainMode) {
        SvnCommandItem item;

        // ── Drain all LocalWrite items (non-blocking) ──
        {
            QMutexLocker locker(&m_queueMutex);
            while (!m_localWriteQueue.isEmpty()) {
                item = m_localWriteQueue.dequeue();
                locker.unlock();

                // Serialize all writes (LocalWrite and HeavyWrite share the same lock)
                if (s_writeSemaphore.tryAcquire(LOCK_WAIT_TIMEOUT_MS)) {
                    processItem(item);
                    s_writeSemaphore.release();
                } else {
                    qWarning() << "[SvnCommandExecutor] LocalWrite timed out waiting for write lock:"
                              << "cmd=" << static_cast<int>(item.command) << "path=" << item.path;
                    emit onTimeout(QString::number(static_cast<int>(item.command)), item.path);
                }

                locker.relock();
            }
        }

        // ── Try one HeavyWrite item ──
        bool gotHeavy = false;
        {
            QMutexLocker locker(&m_queueMutex);
            if (!m_heavyWriteQueue.isEmpty()) {
                item = m_heavyWriteQueue.dequeue();
                gotHeavy = true;
            }
        }

        if (gotHeavy) {
            // Acquire exclusive write lock (with timeout)
            if (s_writeSemaphore.tryAcquire(LOCK_WAIT_TIMEOUT_MS)) {
                processItem(item);
                s_writeSemaphore.release();
            } else {
                qWarning() << "[SvnCommandExecutor] HeavyWrite timed out waiting for write lock:"
                          << "cmd=" << static_cast<int>(item.command) << "path=" << item.path;
                emit onTimeout(QString::number(static_cast<int>(item.command)), item.path);
            }
            continue; // restart loop to drain LocalWrite again
        }

        if (m_drainMode) {
            QMutexLocker locker(&m_drainMutex);
            m_drained = true;
            m_drainCond.wakeOne();
            qDebug() << "[SvnCommandExecutor] Drain complete, worker loop ending";
            break;
        }

        // Both queues empty — wait with timeout
        QMutexLocker locker(&m_queueMutex);
        m_queueCond.wait(&m_queueMutex, 50);
    }
}

// ── Process single command item ────────────────────────────────

void SvnCommandExecutor::maybeRunStaleLockCleanup(const QString &path)
{
    // Throttle: at most once per 60s
    QDateTime now = QDateTime::currentDateTime();
    if (m_lastCleanupAt.isValid() && m_lastCleanupAt.secsTo(now) < 60)
        return;
    m_lastCleanupAt = now;

    if (!m_svnClient || path.isEmpty()) return;
    // svn cleanup <path> clears stale locks in the path's ancestry.
    // Idempotent, fast on clean working copies. Mirrors WPF's
    // SvnService.TryCleanStaleLocks() that runs before every write.
    bool ok = m_svnClient->runSvnBool(
        {QStringLiteral("cleanup"), QStringLiteral("--non-interactive"),
         QStringLiteral("--trust-server-cert"), path},
        QString(), 5000);
    if (ok) {
        qDebug() << "[SvnCommandExecutor] Cleanup OK for" << path;
    } else {
        qDebug() << "[SvnCommandExecutor] Cleanup skipped/failed for" << path;
    }
}

void SvnCommandExecutor::processItem(const SvnCommandItem &item)
{
    if (!m_svnClient) {
        qWarning() << "[SvnCommandExecutor] m_svnClient is null, cannot process item:" << item.path;
        emit onSyncError(QStringLiteral("SVN client not initialized"));
        return;
    }

    // Capture commandError for auth detection. processItem runs on the
    // SvnCommandExecutor worker thread, but SVNClient::commandError is
    // emitted on the thread that owns SVNClient (typically the main thread).
    // The default AutoConnection would queue the slot cross-thread, but
    // the QObject `tmp` lives only on the stack of this call — the queued
    // slot would be discarded when the event loop tries to dispatch it
    // after `tmp` is destroyed. DirectConnection makes the slot fire
    // synchronously on the emitter thread, capturing the error before
    // processItem returns. We accept the cross-thread QString copy
    // because (a) the signal payload is small, (b) it's the only way to
    // reliably read the error from this worker.
    QString capturedError;
    QObject tmp;
    connect(m_svnClient, &SVNClient::commandError, &tmp, [&capturedError](const QString &err) {
        capturedError = err;
    }, Qt::DirectConnection);

    bool success = false;
    QString error;
    int revision = -1;

    // WPF parity: clear stale working copy locks before every write.
    // Throttled to 60s inside the helper.
    {
        QString pathForCleanup = item.path;
        if (item.command == SvnCommand::Update && !item.updatePaths.isEmpty())
            pathForCleanup = item.updatePaths.first();
        maybeRunStaleLockCleanup(pathForCleanup);
    }

    switch (item.command) {
        // ── LocalWrite ──
        case SvnCommand::Add: {
            success = m_svnClient->runSvnBool({QStringLiteral("add"), item.path}, QString(), SVNClient::DEFAULT_TIMEOUT_MS);
            error = success ? QString() : QStringLiteral("svn add failed");
            break;
        }
        case SvnCommand::Delete: {
            success = m_svnClient->runSvnBool({QStringLiteral("delete"), item.path}, QString(), SVNClient::DEFAULT_TIMEOUT_MS);
            error = success ? QString() : QStringLiteral("svn delete failed");
            break;
        }
        case SvnCommand::Move: {
            success = m_svnClient->runSvnBool({QStringLiteral("move"), item.fromPath, item.path}, QString(), SVNClient::DEFAULT_TIMEOUT_MS);
            error = success ? QString() : QStringLiteral("svn move failed");
            break;
        }
        case SvnCommand::Revert: {
            success = m_svnClient->runSvnBool({QStringLiteral("revert"), item.path, QStringLiteral("--recursive")}, QString(), SVNClient::DEFAULT_TIMEOUT_MS);
            error = success ? QString() : QStringLiteral("svn revert failed");
            break;
        }
        case SvnCommand::Resolve: {
            QStringList args = {QStringLiteral("resolve"), item.accept.isEmpty() ? QStringLiteral("--accept=working") : QStringLiteral("--accept=") + item.accept, item.path};
            success = m_svnClient->runSvnBool(args, QString(), SVNClient::DEFAULT_TIMEOUT_MS);
            error = success ? QString() : QStringLiteral("svn resolve failed");
            break;
        }
        case SvnCommand::BreakLock: {
            QStringList args = {QStringLiteral("unlock"), item.path};
            success = m_svnClient->runSvnBool(args, QString(), SVNClient::DEFAULT_TIMEOUT_MS);
            error = success ? QString() : QStringLiteral("svn unlock failed");
            break;
        }

        // ── HeavyWrite ──
        case SvnCommand::Commit: {
            QStringList args = {QStringLiteral("commit"), item.message.isEmpty() ? QStringLiteral("-m ''") : QStringLiteral("-m %1").arg(item.message), item.path};
            success = m_svnClient->runSvnBool(args, QString(), SVNClient::HEAVYWRITE_TIMEOUT_MS);
            error = success ? QString() : QStringLiteral("svn commit failed");
            break;
        }
        case SvnCommand::Update: {
            QStringList args = {QStringLiteral("update")};
            if (!item.updatePaths.isEmpty()) {
                args.append(item.updatePaths);
            } else {
                args.append(item.path);
            }
            args.append(QStringLiteral("--non-interactive"));
            QString output;
            success = m_svnClient->runSvnTimed(args, QString(), SVNClient::HEAVYWRITE_TIMEOUT_MS, &output);
            if (success && output.contains(QStringLiteral("revision"))) {
                QRegularExpression re(QStringLiteral(R"(At revision (\d+))"));
                auto match = re.match(output);
                if (match.hasMatch())
                    revision = match.captured(1).toInt();
                // Emit per-file activity: lines like "U    path/to/file" or "A  /path"
                // SVN update output format: <status><spaces><path>
                // Status chars: A D U C G R
                const QStringList lines = output.split(QChar('\n'));
                for (const QString &line : lines) {
                    if (line.isEmpty()) continue;
                    QChar c = line.at(0);
                    if (c == QChar('A') || c == QChar('U') || c == QChar('D')
                        || c == QChar('C') || c == QChar('G') || c == QChar('R')) {
                        // Skip lines that don't look like status (e.g. "Updating ...")
                        if (line.startsWith(QStringLiteral("Updating "))) continue;
                        // Path is column 5+ (4 spaces, per svn convention)
                        QString filePath = line.mid(5).trimmed();
                        if (!filePath.isEmpty()) {
                            emit m_svnClient->fileTransferActivity(filePath, -1, -1);
                        }
                    }
                }
            } else if (output.isEmpty() && !success) {
                error = QStringLiteral("svn update timed out or failed");
            } else {
                error = output.isEmpty() ? QStringLiteral("svn update failed") : output;
            }
            break;
        }
        case SvnCommand::Checkout: {
            QStringList args = {QStringLiteral("checkout"), item.repoUrl, item.path,
                                 QStringLiteral("--non-interactive")};
            if (!item.username.isEmpty())
                args.append({QStringLiteral("--username"), item.username});
            QString output;
            success = m_svnClient->runSvnTimed(args, QString(), SVNClient::HEAVYWRITE_TIMEOUT_MS, &output);
            if (success && output.contains(QStringLiteral("Checked out revision"))) {
                QRegularExpression re(QStringLiteral(R"(Checked out revision (\d+))"));
                auto match = re.match(output);
                if (match.hasMatch())
                    revision = match.captured(1).toInt();
            } else {
                success = false;
                error = output.isEmpty() ? QStringLiteral("svn checkout timed out or failed") : output;
            }
            break;
        }

        default:
            error = QStringLiteral("Unknown command");
            break;
    }

    // Auth error detected — emit credential expired so the user can
    // re-enter credentials. SVN's own auth cache is invalidated by the
    // server returning 401, so the next command after the user re-enters
    // creds will succeed without us needing to clear the cache first.
    //
    // P3 review fix (M6 + user feedback): the previous implementation
    // called `system("rm -rf ~/.subversion/auth/*")` and retried once
    // before giving up. That "wipe + retry" path was both heavy-handed
    // and redundant — `svn` itself re-prompts when cached creds are
    // rejected, and clearing the cache mid-retry doesn't help because
    // there's no new creds to use yet.
    if (!success && capturedError.startsWith(QStringLiteral("auth_error:"))) {
        qWarning() << "[SvnCommandExecutor] Auth error, requesting credential re-entry:" << item.path;
        emit onAuthError(item.path);
    }

    removeFromDedup(item);
    auto result = success
        ? SvnCommandResult::Ok(item.command, item.path, revision)
        : SvnCommandResult::Fail(item.command, item.path, error);
    emit onCommandCompleted(result);
}

// ── ReadOnly execution ─────────────────────────────────────────

SvnQueryResult SvnCommandExecutor::executeReadOnly(
    SvnCommand cmd,
    const QString &path,
    const QString &fromPath,
    const QString &message,
    const QString &repoUrl,
    const QString &username,
    const QString &password,
    bool depth)
{
    Q_UNUSED(fromPath);
    Q_UNUSED(message);

    if (!m_svnClient) {
        qWarning() << "[SvnCommandExecutor] executeReadOnly called with null m_svnClient";
        return SvnQueryResult::Fail(QStringLiteral("SVN client not initialized"));
    }

    try {
        switch (cmd) {
            case SvnCommand::Info: {
                QString url = m_svnClient->getRepoUrl(path);
                return SvnQueryResult::Ok(url);
            }
            case SvnCommand::Status: {
                auto statusMap = m_svnClient->getStatus(path, depth);
                // Serialize to JSON string
                QStringList parts;
                for (auto it = statusMap.constBegin(); it != statusMap.constEnd(); ++it) {
                    parts.append(it.key() + QStringLiteral(":") + it.value().toString());
                }
                return SvnQueryResult::Ok(parts.join(QStringLiteral(";")));
            }
            case SvnCommand::GetRevision: {
                int rev = m_svnClient->getWorkingCopyRevision(path);
                return SvnQueryResult::Ok(QString::number(rev));
            }
            case SvnCommand::GetHeadRevision: {
                int rev = m_svnClient->getHeadRevision(repoUrl.isEmpty() ? path : repoUrl, username, password);
                return SvnQueryResult::Ok(QString::number(rev));
            }
            case SvnCommand::GetConflictedFiles: {
                auto files = m_svnClient->getConflictedFiles(path);
                return SvnQueryResult::Ok(files.join(QStringLiteral(";")));
            }
            case SvnCommand::GetLastChangedTime: {
                QString time = m_svnClient->getLastChangedTime(path);
                return SvnQueryResult::Ok(time);
            }
            case SvnCommand::IsVersioned: {
                bool ok = m_svnClient->isVersioned(path);
                return SvnQueryResult::Ok(ok ? QStringLiteral("true") : QStringLiteral("false"));
            }
            case SvnCommand::IsValidWorkingCopy: {
                bool ok = m_svnClient->isValidWorkingCopy(path);
                return SvnQueryResult::Ok(ok ? QStringLiteral("true") : QStringLiteral("false"));
            }
            case SvnCommand::TestConnection: {
                bool ok = m_svnClient->testConnection(repoUrl.isEmpty() ? path : repoUrl, username, password);
                return ok ? SvnQueryResult::Ok(QStringLiteral("success"))
                           : SvnQueryResult::Fail(QStringLiteral("connection failed"));
            }
            case SvnCommand::GetServerUpdatePaths: {
                auto paths = m_svnClient->getServerUpdatePaths(path);
                return SvnQueryResult::Ok(paths.join(QStringLiteral(";")));
            }
            default:
                return SvnQueryResult::Fail(QStringLiteral("Unknown ReadOnly command"));
        }
    } catch (const QString &e) {
        return SvnQueryResult::Fail(e);
    } catch (const std::exception &e) {
        return SvnQueryResult::Fail(QString::fromLocal8Bit(e.what()));
    }
}

// ── Deduplication ──────────────────────────────────────────────

QString SvnCommandExecutor::dedupKey(const SvnCommandItem &item) const
{
    QString key = item.path;
    if (item.command == SvnCommand::Update && !item.updatePaths.isEmpty()) {
        QStringList sorted = item.updatePaths;
        sorted.sort();
        QString pathsStr = sorted.join(QStringLiteral(","));
        const int MAX_KEY_LEN = 500;
        if (key.length() + pathsStr.length() + 1 > MAX_KEY_LEN)
            key = key.left(MAX_KEY_LEN - pathsStr.length() - 2) + QStringLiteral("|") + pathsStr;
        else
            key = key + QStringLiteral("|") + pathsStr;
    }
    return key;
}

bool SvnCommandExecutor::tryEnqueueLocalWrite(const SvnCommandItem &item)
{
    QMutexLocker locker(&m_queueMutex);
    m_dedup[dedupKey(item)] = item;
    return true;
}

bool SvnCommandExecutor::tryEnqueueHeavyWrite(const SvnCommandItem &item)
{
    QMutexLocker locker(&m_queueMutex);
    if (item.command == SvnCommand::Checkout) {
        // Checkout always allowed (different repo)
        m_dedup[dedupKey(item)] = item;
        return true;
    }

    if (item.command == SvnCommand::Commit) {
        if (m_dedup.contains(dedupKey(item))) {
            auto existing = m_dedup[dedupKey(item)];
            if (existing.command == SvnCommand::Commit || existing.command == SvnCommand::Update)
                return false; // skip duplicate
        }
        m_dedup[dedupKey(item)] = item;
        return true;
    }

    if (item.command == SvnCommand::Update) {
        if (m_dedup.contains(dedupKey(item))) {
            auto existing = m_dedup[dedupKey(item)];
            if (existing.command == SvnCommand::Update || existing.command == SvnCommand::Commit)
                return false;
        }
        m_dedup[dedupKey(item)] = item;
        return true;
    }

    m_dedup[dedupKey(item)] = item;
    return true;
}

void SvnCommandExecutor::removeFromDedup(const SvnCommandItem &item)
{
    QMutexLocker locker(&m_queueMutex);
    m_dedup.remove(dedupKey(item));
}

} // namespace SVNFileBox
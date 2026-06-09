#include "svncommandexecutor.h"
#include "svnclient.h"
#include <QtCore/QDebug>

namespace SVNFileBox {

QSemaphore SvnCommandExecutor::s_writeSemaphore(1);

SvnCommandExecutor::SvnCommandExecutor(QObject *parent)
    : QObject(parent)
{
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

    if (category == SvnCommandCategory::ReadOnly) {
        return executeReadOnly(cmd, path, fromPath, message, repoUrl, username, password, depth);
    }

    if (category == SvnCommandCategory::LocalWrite) {
        auto item = SvnCommandItem::make(cmd, path, fromPath, QString(), QString(), QString(), QString(), QStringList(), accept);
        if (tryEnqueueLocalWrite(item)) {
            QMutexLocker locker(&m_queueMutex);
            m_localWriteQueue.enqueue(item);
            m_queueCond.wakeOne();
        }
        return SvnQueryResult::Ok(QString());
    }

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

        // ── Drain all LocalWrite items ──
        {
            QMutexLocker locker(&m_queueMutex);
            while (!m_localWriteQueue.isEmpty()) {
                item = m_localWriteQueue.dequeue();
                locker.unlock();

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
            if (s_writeSemaphore.tryAcquire(LOCK_WAIT_TIMEOUT_MS)) {
                processItem(item);
                s_writeSemaphore.release();
            } else {
                qWarning() << "[SvnCommandExecutor] HeavyWrite timed out waiting for write lock:"
                          << "cmd=" << static_cast<int>(item.command) << "path=" << item.path;
                emit onTimeout(QString::number(static_cast<int>(item.command)), item.path);
            }
            continue;
        }

        if (m_drainMode) {
            QMutexLocker locker(&m_drainMutex);
            m_drained = true;
            m_drainCond.wakeOne();
            qDebug() << "[SvnCommandExecutor] Drain complete, worker loop ending";
            break;
        }

        QMutexLocker locker(&m_queueMutex);
        m_queueCond.wait(&m_queueMutex, 50);
    }
}

// ── Process single command item ────────────────────────────────

void SvnCommandExecutor::processItem(const SvnCommandItem &item)
{
    // Each item gets its own SVNClient instance (SharpSVN style).
    // The client is destroyed when this function returns.
    SVNClient client;
    client.setUsername(item.username);
    client.setPassword(item.password);

    // Capture auth errors via DirectConnection on the stack.
    // SVNClient::commandError is emitted on the emitter's thread (worker thread),
    // so DirectConnection fires immediately here.
    QString capturedError;
    QObject tmp;
    connect(&client, &SVNClient::commandError, &tmp,
            [&capturedError](const QString &err) { capturedError = err; },
            Qt::DirectConnection);

    bool success = false;
    QString error;
    int revision = -1;

    // Throttled cleanup before write operations
    QDateTime now = QDateTime::currentDateTime();
    bool shouldCleanup = !m_lastCleanupAt.isValid() || m_lastCleanupAt.secsTo(now) >= 60;
    if (shouldCleanup && !item.path.isEmpty()) {
        m_lastCleanupAt = now;
        client.cleanup(item.path); // ignore failure — cleanup is best-effort
    }

    switch (item.command) {
        case SvnCommand::Add:
            success = client.add(item.path);
            error = success ? QString() : QStringLiteral("svn add failed");
            break;
        case SvnCommand::Delete:
            success = client.remove(item.path);
            error = success ? QString() : QStringLiteral("svn delete failed");
            break;
        case SvnCommand::Move:
            success = client.move(item.fromPath, item.path);
            error = success ? QString() : QStringLiteral("svn move failed");
            break;
        case SvnCommand::Revert:
            success = client.revert(item.path, true);
            error = success ? QString() : QStringLiteral("svn revert failed");
            break;
        case SvnCommand::Resolve:
            success = client.resolveConflict(item.path,
                                            item.accept.isEmpty() ? QStringLiteral("working") : item.accept);
            error = success ? QString() : QStringLiteral("svn resolve failed");
            break;
        case SvnCommand::BreakLock:
            success = client.breakWriteLock(item.path);
            error = success ? QString() : QStringLiteral("svn break-write-lock failed");
            break;
        case SvnCommand::Commit:
            success = client.commit(item.path, item.message);
            error = success ? QString() : QStringLiteral("svn commit failed");
            break;
        case SvnCommand::Update: {
            if (!item.updatePaths.isEmpty()) {
                // Multi-path update: update the root, then each sub-path
                if (!item.path.isEmpty())
                    success = client.update(item.path);
                for (const QString &p : item.updatePaths)
                    success = client.update(p) || success;
            } else {
                success = client.update(item.path);
            }
            // revision not directly available from libsvn without parsing output;
            // callers can use getWorkingCopyRevision after the signal fires.
            error = success ? QString() : QStringLiteral("svn update failed");
            break;
        }
        case SvnCommand::Checkout:
            success = client.checkout(item.repoUrl, item.path);
            error = success ? QString() : QStringLiteral("svn checkout failed");
            break;
        default:
            error = QStringLiteral("Unknown command");
            break;
    }

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

    SVNClient client;
    if (!username.isEmpty()) client.setUsername(username);
    if (!password.isEmpty()) client.setPassword(password);

    try {
        switch (cmd) {
            case SvnCommand::Info: {
                QString url = client.getRepoUrl(path);
                return SvnQueryResult::Ok(url);
            }
            case SvnCommand::Status: {
                auto statusMap = client.getStatus(path, depth);
                QStringList parts;
                for (auto it = statusMap.constBegin(); it != statusMap.constEnd(); ++it)
                    parts.append(it.key() + QStringLiteral(":") + it.value().toString());
                return SvnQueryResult::Ok(parts.join(QStringLiteral(";")));
            }
            case SvnCommand::GetRevision: {
                int rev = client.getWorkingCopyRevision(path);
                return SvnQueryResult::Ok(QString::number(rev));
            }
            case SvnCommand::GetHeadRevision: {
                int rev = client.getHeadRevision(repoUrl.isEmpty() ? path : repoUrl);
                return SvnQueryResult::Ok(QString::number(rev));
            }
            case SvnCommand::GetConflictedFiles: {
                auto files = client.getConflictedFiles(path);
                return SvnQueryResult::Ok(files.join(QStringLiteral(";")));
            }
            case SvnCommand::GetLastChangedTime: {
                QString time = client.getLastChangedTime(path);
                return SvnQueryResult::Ok(time);
            }
            case SvnCommand::IsVersioned: {
                bool ok = client.isVersioned(path);
                return SvnQueryResult::Ok(ok ? QStringLiteral("true") : QStringLiteral("false"));
            }
            case SvnCommand::IsValidWorkingCopy: {
                bool ok = client.isValidWorkingCopy(path);
                return SvnQueryResult::Ok(ok ? QStringLiteral("true") : QStringLiteral("false"));
            }
            case SvnCommand::TestConnection: {
                bool ok = client.testConnection(repoUrl.isEmpty() ? path : repoUrl);
                return ok ? SvnQueryResult::Ok(QStringLiteral("success"))
                          : SvnQueryResult::Fail(QStringLiteral("connection failed"));
            }
            case SvnCommand::GetServerUpdatePaths: {
                auto paths = client.getServerUpdatePaths(path);
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
        m_dedup[dedupKey(item)] = item;
        return true;
    }

    if (item.command == SvnCommand::Commit) {
        if (m_dedup.contains(dedupKey(item))) {
            auto existing = m_dedup[dedupKey(item)];
            if (existing.command == SvnCommand::Commit || existing.command == SvnCommand::Update)
                return false;
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
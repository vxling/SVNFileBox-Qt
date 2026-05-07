#include "syncengine.h"
#include "svn/svnclient.h"
#include "sync/syncrecordservice.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QThread>
#include <QRegularExpression>

SyncEngine::SyncEngine(QObject *parent)
    : QObject(parent)
    , m_syncing(false)
{
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(5000);
    connect(m_debounceTimer, &QTimer::timeout, this, &SyncEngine::onDebounceTimer);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(m_pollIntervalSec * 1000);
    connect(m_pollTimer, &QTimer::timeout, this, &SyncEngine::onPollTimer);

    m_fullSyncTimer = new QTimer(this);
    m_fullSyncTimer->setInterval(30 * 60 * 1000);
    connect(m_fullSyncTimer, &QTimer::timeout, this, &SyncEngine::onFullSyncTimer);
}

SyncEngine::~SyncEngine()
{
    stopSync();
}

void SyncEngine::watchPath(const QString &path)
{
    if (!m_syncing || path.isEmpty()) return;

    // Clear existing watcher and recreate
    if (m_watcher) {
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &SyncEngine::onFileChanged);

    // Always watch the current path itself
    if (QDir(path).exists()) {
        m_watcher->addPath(path);
    }

    // Watch direct child directories (one level only, not recursive)
    QDir dir(path);
    if (dir.exists()) {
        for (const QString &sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString subPath = path + "/" + sub;
            if (QFileInfo(subPath).isDir()) {
                m_watcher->addPath(subPath);
            }
        }
    }

    qDebug() << "[SyncEngine] Watching path:" << path
             << "plus" << m_watcher->files().size() - 1 << "sub-directories";
}

void SyncEngine::startSync(const QString &repoName, const QString &localPath, const QString &remoteUrl,
                           const QString &username, const QString &password)
{
    m_repoName = repoName;
    m_localPath = localPath;
    m_remoteUrl = remoteUrl;
    m_username = username;
    m_password = password;
    m_syncing = true;

    // Watch the repo root initially
    watchPath(m_localPath);

    m_pollTimer->start();
    m_fullSyncTimer->start();

    emit syncStarted();
    emit syncNotification("同步已启动: " + m_repoName);
    qDebug() << "[SyncEngine] Started sync for" << m_repoName << "at" << m_localPath;
}

void SyncEngine::stopSync()
{
    m_syncing = false;
    m_pollTimer->stop();
    m_fullSyncTimer->stop();
    m_debounceTimer->stop();

    if (m_watcher) {
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }
    qDebug() << "[SyncEngine] Stopped sync";
}

void SyncEngine::syncNow()
{
    if (!m_syncing || m_localPath.isEmpty()) return;
    qDebug() << "[SyncEngine] Manual syncNow triggered";

    // Upload: commit pending local changes
    retryPending();
    pollServer();
    emit filesChanged();
}

QString SyncEngine::status() const
{
    if (m_pausedByConflict) return "conflict";
    return m_syncing ? "running" : "stopped";
}

QStringList SyncEngine::getConflictedFiles() const
{
    if (!m_svnClient || m_localPath.isEmpty()) return {};
    return m_svnClient->getConflictedFiles(m_localPath);
}

void SyncEngine::resolveConflict(const QString &accept)
{
    if (!m_svnClient || m_localPath.isEmpty()) return;

    QStringList files = m_svnClient->getConflictedFiles(m_localPath);
    for (const QString &f : files) {
        // Build absolute path: m_localPath + "/" + f
        QString absPath = f.contains(m_localPath) ? f : m_localPath + "/" + f;
        m_svnClient->resolveConflict(absPath, accept);
    }

    m_pausedByConflict = false;
    emit syncNotification(QString("冲突已解决（%1）").arg(accept == "mine-conflict" ? "保留本地" : "使用服务器"));
    emit filesChanged();
}

void SyncEngine::resolveConflictForFile(const QString &filePath, const QString &accept)
{
    if (!m_svnClient || filePath.isEmpty()) return;
    QString absPath = filePath.contains(m_localPath) ? filePath : m_localPath + "/" + filePath;
    m_svnClient->resolveConflict(absPath, accept);
    qDebug() << "[SyncEngine] Resolved conflict for:" << absPath << "with" << accept;
}

void SyncEngine::onFileChanged(const QString &path)
{
    if (!m_syncing) return;
    if (path.isEmpty()) return;

    // Ignore .svn internal files
    if (path.contains("/.svn/") || path.endsWith(".svn")) return;

    qDebug() << "[SyncEngine] File changed:" << path;
    addPending(path);

    if (!m_debounceTimer->isActive()) {
        m_debounceTimer->start();
    }
}

void SyncEngine::onDebounceTimer()
{
    if (!m_syncing) return;
    if (m_pausedByConflict) return;

    // Check for conflicts before committing
    if (!m_svnClient || m_localPath.isEmpty()) return;
    QStringList conflicts = m_svnClient->getConflictedFiles(m_localPath);
    if (!conflicts.isEmpty()) {
        qDebug() << "[SyncEngine] Conflicts detected, pausing sync";
        m_pausedByConflict = true;
        emit conflictDetected(conflicts);
        return;
    }

    qDebug() << "[SyncEngine] Debounce timer fired, committing pending files";
    retryPending();
    emit filesChanged();
}

void SyncEngine::onPollTimer()
{
    if (!m_syncing || m_localPath.isEmpty()) return;
    if (m_pausedByConflict) return;
    qDebug() << "[SyncEngine] Poll timer fired";
    pollServer();
}

void SyncEngine::onFullSyncTimer()
{
    if (!m_syncing || m_localPath.isEmpty()) return;
    if (m_pausedByConflict) return;
    qDebug() << "[SyncEngine] Full sync timer fired";
    fullScan();
}

void SyncEngine::pollServer()
{
    if (!m_svnClient || m_localPath.isEmpty()) return;

    int localRev = m_svnClient->getWorkingCopyRevision(m_localPath);
    int serverRev = m_remoteUrl.isEmpty() ? localRev
                                          : m_svnClient->getHeadRevision(m_remoteUrl);

    qDebug() << "[SyncEngine] PollCheck: local=" << localRev << "server=" << serverRev;
    if (serverRev <= localRev || serverRev < 0) return;

    qDebug() << "[SyncEngine] Server has updates, updating...";
    bool ok = m_svnClient->update(m_localPath);
    if (ok) handleConflicts();

    QString msg;
    QString result;
    if (ok) {
        int updated = serverRev - localRev;
        msg = QString("从服务器更新 %1 个版本").arg(updated);
        result = "Success";
        if (m_recordService)
            m_recordService->addRecord(m_repoName, m_localPath, "Update", result, msg);
        emit syncNotification(msg);
        emit filesChanged();
    } else {
        msg = "更新失败";
        result = "Failed";
        if (m_recordService)
            m_recordService->addRecord(m_repoName, m_localPath, "Update", result, msg);
        emit syncNotification(msg);
    }
}

void SyncEngine::fullScan()
{
    if (!m_svnClient || m_localPath.isEmpty()) return;

    qDebug() << "[SyncEngine] Full scan started";

    // Commit any pending retries first
    retryPending();

    // Check all tracked files for local modifications
    QString statusOutput = m_svnClient->getStatus(m_localPath);
    if (!statusOutput.isEmpty()) {
        qDebug() << "[SyncEngine] Full scan found changes, committing...";
        // getStatus returns output that may contain modified files
        // Retry pending again to pick up any files that were missed
        retryPending();
    }

    // Poll server for updates
    pollServer();

    qDebug() << "[SyncEngine] Full scan done";
}

void SyncEngine::commitFile(const QString &filePath)
{
    if (!m_svnClient || filePath.isEmpty()) return;
    if (m_pausedByConflict) {
        qDebug() << "[SyncEngine] Paused by conflict, skipping commit for:" << filePath;
        return;
    }

    bool exists = QFile::exists(filePath) || QDir(filePath).exists();
    QString parentDirPath = parentDir(filePath);
    QString fileName = filePath.split("/").last();

    if (!exists) {
        // File was deleted
        // Check if SVN tracked it
        if (!isSvnManaged(filePath)) {
            qDebug() << "[SyncEngine] Deleted file was never tracked, skipping:" << filePath;
            return;
        }
        bool ok = m_svnClient->remove(filePath);
        if (ok) {
            QString msg = "[Auto-sync] Delete: " + fileName;
            bool committed = m_svnClient->commit(parentDirPath, msg);
            if (m_recordService) {
                m_recordService->addRecord(m_repoName, fileName, "Delete",
                    committed ? "Success" : "Failed", committed ? "" : "Commit failed");
            }
            emit syncNotification(committed ? QString("已同步删除: %1").arg(fileName)
                                            : QString("删除同步失败: %1").arg(fileName));
        }
        return;
    }

    // File exists - add if needed, then commit
    QString operation = "Update";
    if (!isSvnManaged(filePath)) {
        m_svnClient->add(filePath);
        operation = "Add";
    }

    QString msg = QString("[Auto-sync] %1: %2").arg(operation).arg(fileName);
    bool committed = m_svnClient->commit(parentDirPath, msg);
    if (committed) {
        handleConflicts();
        qDebug() << "[SyncEngine] Committed:" << filePath;
        if (m_recordService)
            m_recordService->addRecord(m_repoName, fileName, operation, "Success", msg);
        emit syncNotification(QString("已同步: %1").arg(fileName));
    } else {
        qDebug() << "[SyncEngine] Commit failed for:" << filePath;
        if (m_recordService)
            m_recordService->addRecord(m_repoName, fileName, operation, "Failed", "Commit returned non-zero");
        emit syncNotification(QString("同步失败: %1").arg(fileName));
    }
}

int SyncEngine::handleConflicts()
{
    if (!m_svnClient || m_localPath.isEmpty()) return 0;

    // SVN conflict extensions: .mine, .rOLD, .rNEW, .r{rev}
    // Also .orig, .rej (patch conflicts)
    QStringList conflictExts = {".mine", ".rOLD", ".rNEW", ".r*", ".orig", ".rej"};

    QDir dir(m_localPath);
    int count = 0;

    // Scan for conflict files and resolve them
    // Resolution strategy: keep .mine (local changes), remove .r* artifacts
    for (const QString &entry : dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool isConflict = false;
        for (const QString &ext : conflictExts) {
            if (ext == ".r*") {
                // Match .r123 pattern
                if (entry.contains(QRegularExpression("\\.r\\d+"))) {
                    isConflict = true;
                    break;
                }
            } else if (entry.endsWith(ext)) {
                isConflict = true;
                break;
            }
        }
        if (isConflict) {
            QString filePath = m_localPath + "/" + entry;
            // For .r* files: safe to delete (remote old versions)
            // For .mine files: keep as backup (already handled by SVN merge)
            // For .orig: can be removed
            if (entry.contains(QRegularExpression("\\.r\\d+")) || entry.endsWith(".orig") || entry.endsWith(".rej")) {
                if (QFile::remove(filePath)) {
                    qDebug() << "[SyncEngine] Removed conflict artifact:" << filePath;
                    ++count;
                }
            }
        }
    }

    if (count > 0) {
        QString msg = QString("自动清理 %1 个冲突文件").arg(count);
        emit syncNotification(msg);
        if (m_recordService)
            m_recordService->addRecord(m_repoName, m_localPath, "ConflictResolve", "Success", msg);
    }

    return count;
}

void SyncEngine::retryPending()
{
    QSet<QString> files;
    {
        QMutexLocker locker(&m_pendingMutex);
        files = m_pendingFiles;
        m_pendingFiles.clear();
    }

    for (const QString &f : files) {
        if (!QFile::exists(f) && !QDir(f).exists()) {
            // File was deleted - still need to tell SVN
            if (isSvnManaged(f)) {
                commitFile(f);
            }
            continue;
        }
        commitFile(f);
    }
}

void SyncEngine::addPending(const QString &path)
{
    QMutexLocker locker(&m_pendingMutex);
    m_pendingFiles.insert(path);
}

bool SyncEngine::isSvnManaged(const QString &path) const
{
    QString dir = QFileInfo(path).absolutePath();
    while (!dir.isEmpty()) {
        if (QDir(dir).exists(".svn")) return true;
        if (dir == QFileInfo(dir).absolutePath()) break;
        dir = QFileInfo(dir).absolutePath();
    }
    return false;
}

QString SyncEngine::parentDir(const QString &filePath) const
{
    int lastSlash = filePath.lastIndexOf("/");
    if (lastSlash <= 0) return m_localPath;
    return filePath.left(lastSlash);
}

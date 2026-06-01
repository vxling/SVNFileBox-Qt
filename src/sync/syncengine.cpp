#include "syncengine.h"
#include "ignorepattern.h"
#include "svn/svnclient.h"
#include "sync/syncrecordservice.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QThread>
#include <QRegularExpression>
#include <QDateTime>
#include <QSet>
#include <QVariantMap>
#include <QDirIterator>

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
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &SyncEngine::onDirChanged);
    // Qt 6.7 has no errorOccurred signal (added in 6.8). Reconnection is
    // triggered by onDirChanged re-walking the tree.

    if (!QDir(path).exists()) return;

    // Recursive watch: add all subdirectories. QFileSystemWatcher is not
    // recursive on most platforms (Linux inotify requires explicit add per
    // dir), so we walk the tree ourselves. Mirrors WPF FileSystemWatcher
    // which IS recursive.
    addPathRecursive(path);

    qDebug() << "[SyncEngine] Watching path:" << path
             << "directories:" << m_watcher->directories().size();
}

void SyncEngine::addPathRecursive(const QString &rootPath)
{
    if (!m_watcher) return;
    QDir root(rootPath);
    if (!root.exists()) return;

    // Add the root itself
    m_watcher->addPath(rootPath);

    QDirIterator it(rootPath,
                    QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString sub = it.next();
        // Skip .svn dirs — SVN manages them itself and they change a lot
        if (sub.contains(QStringLiteral("/.svn")) || sub.endsWith(QStringLiteral("/.svn")))
            continue;
        m_watcher->addPath(sub);
    }
}

void SyncEngine::startSync(const QString &repoName, const QString &localPath, const QString &remoteUrl,
                           const QString &username, const QString &password)
{
    // P3 review fix (M5): all writes to m_repoName/m_localPath/m_remoteUrl
    // are serialized through m_stateMutex so a concurrent setRepoName()
    // from the main thread can't tear the QString's COW pointer.
    {
        QMutexLocker locker(&m_stateMutex);
        m_repoName = repoName;
        m_localPath = localPath;
        m_remoteUrl = remoteUrl;
        m_username = username;
        m_password = password;
    }
    m_syncing = true;

    // Watch the repo root initially
    watchPath(m_localPath);

    m_pollTimer->start();
    m_fullSyncTimer->start();

    // Poll server first, then scan & commit local changes
    pollServer();
    scanAndCommit();

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

    scanAndCommit();
    pollServer();
    emit filesChanged();
}

void SyncEngine::scanAndCommit()
{
    if (!m_svnClient || m_localPath.isEmpty()) return;

    // Prevent re-entry (WPF uses Interlocked.CompareExchange)
    {
        QMutexLocker locker(&m_scanMutex);
        if (m_scanning) {
            qDebug() << "[SyncEngine] ScanAndCommit already in progress, skipping";
            return;
        }
        m_scanning = true;
    }

    // Commit any pending retries first
    retryPending();

    // Scan local svn status --xml --depth=infinity
    QVariantMap statuses = m_svnClient->getStatus(m_localPath, true);
    if (statuses.isEmpty()) {
        qDebug() << "[SyncEngine] ScanAndCommit: no changes to commit";
        {
            QMutexLocker locker(&m_scanMutex);
            m_scanning = false;
        }
        return;
    }

    QString repoRoot = m_localPath;

    // Separate unversioned files from versioned changes
    QStringList unversionedFiles;
    QVariantMap versionedChanges;
    for (auto it = statuses.begin(); it != statuses.end(); ++it) {
        QString path = it.key();
        QString status = it.value().toString();
        if (status == "unversioned") {
            unversionedFiles.append(path);
        } else if (status != "conflicted" && status != "treeconflicted") {
            versionedChanges[path] = status;
        }
    }

    // Add all unversioned files (filter temp files)
    for (const QString &filePath : unversionedFiles) {
        if (isTempFile(filePath)) continue;
        m_svnClient->add(filePath);
        qDebug() << "[SyncEngine] ScanAndCommit: enqueued Add for unversioned:" << filePath;
    }

    if (versionedChanges.isEmpty()) {
        {
            QMutexLocker locker(&m_scanMutex);
            m_scanning = false;
        }
        return;
    }

    // Group by parent directory
    // Normalize repoRoot to strip trailing slash
    QString normRoot = repoRoot;
    if (normRoot.endsWith('/')) normRoot.chop(1);

    // Map: parentDir -> list of file paths in that dir
    QMap<QString, QStringList> dirGroups;
    for (auto it = versionedChanges.begin(); it != versionedChanges.end(); ++it) {
        QString path = it.key();
        // Only include files inside repo root
        if (!path.startsWith(normRoot + '/') && path != normRoot) continue;

        // Strip repo root prefix to get relative path
        QString relPath = path;
        if (relPath.startsWith(normRoot + '/'))
            relPath = relPath.mid(normRoot.length() + 1);

        // Determine parent directory
        int lastSlash = relPath.lastIndexOf('/');
        QString parentDir = lastSlash > 0 ? relPath.left(lastSlash) : QString();
        dirGroups[parentDir].append(relPath);
    }

    qDebug() << "[SyncEngine] ScanAndCommit: committing" << dirGroups.size() << "dirs,"
             << versionedChanges.size() << "files (+" << unversionedFiles.size() << "unversioned)";

    // Process deepest dirs first to avoid "out of date" errors
    QList<QString> sortedDirs = dirGroups.keys();
    std::sort(sortedDirs.begin(), sortedDirs.end(), [](const QString &a, const QString &b) {
        return a.count('/') > b.count('/');
    });

    for (const QString &relParent : sortedDirs) {
        QStringList filesInDir = dirGroups.value(relParent);
        QString absParent = relParent.isEmpty() ? normRoot : normRoot + '/' + relParent;
        QString commitPath = absParent;

        QString msg;
        if (filesInDir.size() == 1) {
            msg = "Auto-sync: " + filesInDir.first();
        } else {
            msg = "Auto-sync: " + QString::number(filesInDir.size()) + " files in " + QFileInfo(absParent).fileName();
        }

        qDebug() << "[SyncEngine] ScanAndCommit: committing dir:" << commitPath << "with" << filesInDir.size() << "files";
        m_svnClient->commit(commitPath, msg);
    }

    {
        QMutexLocker locker(&m_scanMutex);
        m_scanning = false;
    }

    emit syncNotification("批量同步完成");
    emit filesChanged();
}

QString SyncEngine::status() const
{
    // P3 review fix (S2): lock for one-line read of m_pausedByConflict.
    // Called from QML binding, may run concurrently with onDebounceTimer().
    bool paused;
    {
        QMutexLocker locker(&m_stateMutex);
        paused = m_pausedByConflict;
    }
    if (paused) return "conflict";
    return m_syncing ? "running" : "stopped";
}

QStringList SyncEngine::getConflictedFiles() const
{
    if (!m_svnClient || m_localPath.isEmpty()) return {};
    return m_svnClient->getConflictedFiles(m_localPath);
}

QVariantList SyncEngine::getConflictedFileInfo() const
{
    if (!m_svnClient || m_localPath.isEmpty()) return {};
    QStringList files = m_svnClient->getConflictedFiles(m_localPath);
    QVariantList result;
    for (const QString &rel : files) {
        QString absPath = rel.contains(m_localPath) ? rel : m_localPath + "/" + rel;
        QVariantMap info;
        info["path"] = rel;

        // Get base info via `svn info` for revision + times
        QString infoXml = m_svnClient->getInfo(absPath);
        // Local mtime via QFileInfo
        QFileInfo fi(absPath);
        QDateTime localMtime = fi.exists() ? fi.lastModified() : QDateTime();

        // Determine kind: tree conflict has "treeconflicted" status from
        // getStatusString. Try that first.
        QString statusStr = m_svnClient->getStatusString(absPath);
        QString kind = "text";
        if (statusStr == "treeconflicted" || statusStr == "Tree conflicted")
            kind = "tree";

        // Parse commit revision from info xml if present
        int commitRev = -1;
        int incomingRev = -1;
        if (!infoXml.isEmpty()) {
            // Look for <entry ... revision="N" ...>
            QRegularExpression revRe(QStringLiteral(R"X([Rr]evision="(\d+)")X"));
            auto m1 = revRe.match(infoXml);
            if (m1.hasMatch()) commitRev = m1.captured(1).toInt();
        }

        info["kind"] = kind;
        info["localModifiedTime"] = localMtime;
        info["serverModifiedTime"] = QDateTime();  // server time not available without log
        info["selectedResolution"] = QString();
        info["baseRevision"] = commitRev;
        info["incomingRevision"] = incomingRev;
        result.append(info);
    }
    return result;
}

void SyncEngine::resolveConflict(const QString &accept)
{
    if (!m_svnClient || m_localPath.isEmpty()) return;

    QStringList files = m_svnClient->getConflictedFiles(m_localPath);
    for (const QString &f : files) {
        QString absPath = f.contains(m_localPath) ? f : m_localPath + "/" + f;
        if (accept == "keep-both") {
            // KeepBoth: backup local as .local-backup-*, then accept server
            QString backupPath = absPath + ".local-backup-" + QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmss");
            if (QFile::copy(absPath, backupPath)) {
                qDebug() << "[SyncEngine] KeepBoth: backed up" << absPath << "→" << backupPath;
            }
            // Tree conflicts can only be resolved with Working (local), so use that
            QString svnAccept = "theirs-conflict"; // default for text conflicts
            m_svnClient->resolveConflict(absPath, svnAccept);
        } else {
            m_svnClient->resolveConflict(absPath, accept);
        }
    }

    // P3 review fix (S2): lock for write of m_pausedByConflict.
    {
        QMutexLocker locker(&m_stateMutex);
        m_pausedByConflict = false;
    }
    QString msg = accept == "keep-both" ? "保留两者（备份后接受服务器）"
                  : accept == "mine-conflict" ? "保留本地" : "使用服务器";
    emit syncNotification(QString("冲突已解决（%1）").arg(msg));
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

void SyncEngine::onDirChanged(const QString &path)
{
    if (!m_syncing) return;
    if (path.isEmpty()) return;
    // Skip .svn internal
    if (path.contains("/.svn") || path.endsWith(".svn")) return;

    // A subdirectory was added/removed/renamed. Re-walk our root and
    // re-add any missing directories to the watcher, then schedule a sync
    // since a new dir probably means new files. Mirrors WPF FSW's
    // Renamed/Created events.
    if (m_watcher && !m_localPath.isEmpty()) {
        QStringList currentDirs = m_watcher->directories();
        // Re-add anything missing by walking the tree again
        addPathRecursive(m_localPath);
        Q_UNUSED(currentDirs);
    }

    if (!m_debounceTimer->isActive()) {
        m_debounceTimer->start();
    }
}

void SyncEngine::onWatcherError(int err)
{
    Q_UNUSED(err);
    qWarning() << "[SyncEngine] FileWatcher error, scheduling reconnect";
    m_watcherRetryCount = 0;
    reconnectWatcher();
}

void SyncEngine::reconnectWatcher()
{
    if (m_watcherRetryCount >= 3) {
        qWarning() << "[SyncEngine] FileWatcher reconnect giving up after 3 tries";
        return;
    }
    int delayMs = (m_watcherRetryCount == 0) ? 5000 : 10000;
    m_watcherRetryCount++;

    QTimer::singleShot(delayMs, this, [this]() {
        if (!m_syncing || m_localPath.isEmpty()) return;
        if (m_watcher) {
            m_watcher->deleteLater();
            m_watcher = nullptr;
        }
        m_watcher = new QFileSystemWatcher(this);
        connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &SyncEngine::onFileChanged);
        connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &SyncEngine::onDirChanged);
        addPathRecursive(m_localPath);
        qDebug() << "[SyncEngine] FileWatcher reconnected (attempt" << m_watcherRetryCount << ")";
    });
}

void SyncEngine::onDebounceTimer()
{
    if (!m_syncing) return;
    // P3 review fix (S2): lock for read of m_pausedByConflict.
    {
        QMutexLocker locker(&m_stateMutex);
        if (m_pausedByConflict) return;
    }

    // Check for conflicts before committing
    if (!m_svnClient || m_localPath.isEmpty()) return;
    QStringList conflicts = m_svnClient->getConflictedFiles(m_localPath);
    if (!conflicts.isEmpty()) {
        qDebug() << "[SyncEngine] Conflicts detected, pausing sync";
        // P3 review fix (S2): lock for write of m_pausedByConflict.
        {
            QMutexLocker locker(&m_stateMutex);
            m_pausedByConflict = true;
        }
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
    // P3 review fix (S2): lock for read of m_pausedByConflict.
    {
        QMutexLocker locker(&m_stateMutex);
        if (m_pausedByConflict) return;
    }
    qDebug() << "[SyncEngine] Poll timer fired";
    pollServer();
}

void SyncEngine::onFullSyncTimer()
{
    if (!m_syncing || m_localPath.isEmpty()) return;
    // P3 review fix (S2): lock for read of m_pausedByConflict.
    {
        QMutexLocker locker(&m_stateMutex);
        if (m_pausedByConflict) return;
    }
    qDebug() << "[SyncEngine] Full sync timer fired";
    scanAndCommit();
    pollServer();
}

void SyncEngine::pollServer()
{
    if (!m_svnClient || m_localPath.isEmpty()) return;

    // ── Stale guard ───────────────────────────────────────────
    // No server updates for 35 consecutive polls → force a full update
    // to recover from missed webhooks or clock skew.
    // Initial poll (_staleCounter==1) also triggers a forced full update.
    m_staleCounter++;
    if (m_staleCounter > 35) {
        qDebug() << "[SyncEngine] Stale for" << m_staleCounter << "polls, forcing full update...";
        emit syncNotification(QStringLiteral("连续无更新次数过多，正在强制全量更新..."));
        bool ok = m_svnClient->update(m_localPath);
        if (ok) {
            handleConflicts();
            emit syncNotification(QStringLiteral("强制全量更新完成"));
        } else {
            emit syncNotification(QStringLiteral("强制全量更新失败"));
            if (m_recordService)
                m_recordService->addRecord(m_repoName, m_localPath, "Update", "Failed", "Stale refresh failed");
        }
        emit filesChanged();
        m_staleCounter = 1;
        return;
    }

    // ── Repair incomplete working copy ─────────────────────────
    if (m_svnClient->hasIncompleteWorkingCopy(m_localPath)) {
        qWarning() << "[SyncEngine] Incomplete working copy detected, repairing...";
        emit syncNotification(QStringLiteral("检测到工作副本损坏，正在修复..."));
        bool ok = m_svnClient->update(m_localPath);
        if (ok) {
            emit syncNotification(QStringLiteral("已修复工作副本中的 incomplete 状态"));
        } else {
            emit syncNotification(QStringLiteral("修复 incomplete 失败"));
            if (m_recordService)
                m_recordService->addRecord(m_repoName, m_localPath, "Update", "Failed",
                    "Repair update failed: incomplete working copy");
        }
        handleConflicts();
        emit filesChanged();
        return;
    }

    // ── Normal server → local update ──────────────────────────
    int localRev = m_svnClient->getWorkingCopyRevision(m_localPath);
    int serverRev = m_remoteUrl.isEmpty() ? localRev
                                          : m_svnClient->getHeadRevision(m_remoteUrl);

    qDebug() << "[SyncEngine] PollCheck: local=" << localRev << "server=" << serverRev;
    if (serverRev <= localRev || serverRev < 0) return;

    qDebug() << "[SyncEngine] Server has updates, updating...";

    // P1-8: UpdateInChunks — get remote-changed paths, group by parent dir, update per dir
    QStringList remotePaths = m_svnClient->getServerUpdatePaths(m_localPath);
    qDebug() << "[SyncEngine] GetServerUpdatePaths returned" << remotePaths.size() << "paths";

    QSet<QString> dirs;
    for (const QString &p : remotePaths) {
        QString normalized = p;
        // Strip repo root prefix if present (relative path)
        if (normalized.startsWith(m_localPath + "/")) {
            normalized = normalized.mid(m_localPath.length() + 1);
        }
        // Get parent dir; root file maps to repo root
        int lastSlash = normalized.lastIndexOf("/");
        QString parent = lastSlash > 0 ? normalized.left(lastSlash) : QString();
        // Collect unique dirs (store relative to repo root), repo root itself is ""
        dirs.insert(parent);
    }

    bool allSuccess = true;
    QStringList sortedDirs = dirs.values();
    // Sort deepest-first (more files first reduces redundant updates)
    std::sort(sortedDirs.begin(), sortedDirs.end(), [](const QString &a, const QString &b) {
        return a.count('/') > b.count('/');
    });

    // Always update repo root first
    bool rootOk = m_svnClient->update(m_localPath);
    if (!rootOk) allSuccess = false;

    // Then update each unique subdirectory
    for (const QString &relDir : sortedDirs) {
        QString absDir = relDir.isEmpty() ? m_localPath : m_localPath + "/" + relDir;
        qDebug() << "[SyncEngine] Updating chunk:" << absDir;
        if (!m_svnClient->update(absDir)) {
            allSuccess = false;
            qWarning() << "[SyncEngine] Update chunk failed:" << absDir;
        }
    }

    if (allSuccess) handleConflicts();

    // Successful update resets stale counter
    m_staleCounter = 1;

    QString msg;
    QString result;
    if (allSuccess) {
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
    QString statusOutput = m_svnClient->getStatusString(m_localPath);
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
    // P3 review fix (S2): lock for read of m_pausedByConflict.
    {
        QMutexLocker locker(&m_stateMutex);
        if (m_pausedByConflict) {
            qDebug() << "[SyncEngine] Paused by conflict, skipping commit for:" << filePath;
            return;
        }
    }
    if (isPathIgnored(filePath)) {
        qDebug() << "[SyncEngine] Ignored by pattern, skipping commit for:" << filePath;
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

    // P3 review fix (M3): back up local .mine files before removing them.
    // After resolution, .mine holds the user's pre-conflict edits; deleting
    // silently is data loss if the user later realises they need it. Keep
    // copies in ~/.svnfilebox/conflict_backups/<basename>-<ts>.mine.
    QString backupDir = QDir::homePath() + "/.svnfilebox/conflict_backups";
    QDir().mkpath(backupDir);

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
            // For .mine files: back up then delete (preserve user's edits)
            // For .orig/.rej: can be removed
            if (entry.contains(QRegularExpression("\\.r\\d+")) || entry.endsWith(".orig") || entry.endsWith(".rej")) {
                if (QFile::remove(filePath)) {
                    qDebug() << "[SyncEngine] Removed conflict artifact:" << filePath;
                    ++count;
                }
            } else if (entry.endsWith(".mine")) {
                QString backupPath = QString("%1/%2-%3.mine")
                    .arg(backupDir, entry.left(entry.length() - 5),  // strip ".mine"
                         QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz"));
                if (QFile::copy(filePath, backupPath)) {
                    if (QFile::remove(filePath)) {
                        qDebug() << "[SyncEngine] Backed up .mine and removed:" << filePath << "->" << backupPath;
                        ++count;
                    } else {
                        qWarning() << "[SyncEngine] Backed up but failed to remove .mine:" << filePath;
                    }
                } else {
                    qWarning() << "[SyncEngine] Failed to back up .mine, leaving in place:" << filePath
                               << "(copy to" << backupPath << "failed)";
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
    CommitQueue &queue = CommitQueue::instance();
    QList<CommitQueue::Item> pending = queue.resolve();
    if (pending.isEmpty()) return;

    queue.markInProgress(pending);
    for (const CommitQueue::Item &it : pending) {
        if (!QFile::exists(it.path) && !QDir(it.path).exists()) {
            if (isSvnManaged(it.path)) {
                commitFile(it.path);
            }
            continue;
        }
        commitFile(it.path);
    }
    queue.markCommitted(pending);
}

void SyncEngine::addPending(const QString &path)
{
    if (isTempFile(path)) return;
    CommitQueue::instance().enqueue(path, CommitQueue::OpModify);
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

void SyncEngine::setFileWatcher(QFileSystemWatcher *watcher)
{
    m_fileWatcher = watcher;
}

void SyncEngine::DisableFileWatcher()
{
    if (m_fileWatcher) {
        const QStringList paths = m_fileWatcher->files() + m_fileWatcher->directories();
        for (const QString &p : paths) {
            m_fileWatcher->removePath(p);
        }
    }
}

void SyncEngine::ReEnableFileWatcher()
{
    if (m_fileWatcher) {
        const QStringList paths = m_fileWatcher->files() + m_fileWatcher->directories();
        for (const QString &p : paths) {
            if (!m_fileWatcher->addPath(p)) {
                qDebug() << "[SyncEngine] Failed to re-add path:" << p;
            }
        }
    }
}

bool SyncEngine::isTempFile(const QString &path) const
{
    QString name = QFileInfo(path).fileName();
    // Office temp: ~$*.doc*, ~$*.xls*, ~$*.ppt*, etc.
    if (name.startsWith(QLatin1String("~$"))) return true;
    // macOS
    if (name == QLatin1String(".DS_Store")) return true;
    if (name == QLatin1String("._") + name.mid(2)) return true; // ._foo (AppleDouble)
    // Vim
    if (name.endsWith(QLatin1String(".swp"))) return true;
    if (name.endsWith(QLatin1String(".swo"))) return true;
    if (name.endsWith(QLatin1String("~"))) return true;
    // Emacs
    if (name.endsWith(QLatin1String("~"))) return true;
    if (name.startsWith(QLatin1String("#")) && name.endsWith(QLatin1String("#"))) return true;
    // Patch conflicts
    if (name.endsWith(QLatin1String(".orig"))) return true;
    if (name.endsWith(QLatin1String(".rej"))) return true;
    // General tmp
    if (name.endsWith(QLatin1String(".tmp"))) return true;
    if (name.endsWith(QLatin1String(".temp"))) return true;
    // Unix core dumps
    if (name.startsWith(QLatin1String("core.")) && name.mid(5).toLongLong() > 0) return true;
    return false;
}

bool SyncEngine::isPathIgnored(const QString &path) const
{
    if (m_ignoreRegexes.isEmpty()) return false;
    if (path.isEmpty()) return false;

    QString name = QFileInfo(path).fileName();
    QString relPath = path;
    if (!m_localPath.isEmpty() && relPath.startsWith(m_localPath)) {
        relPath = relPath.mid(m_localPath.size());
        if (relPath.startsWith(QLatin1Char('/'))) relPath = relPath.mid(1);
    }
    return SVNFileBox::matchIgnore(m_ignoreRegexes, name, relPath);
}

void SyncEngine::setRepoName(const QString &name)
{
    // P3 review fix (M5): update the cached name so subsequent sync records
    // and notifications are tagged with the new repo name. Lock guards the
    // QString's COW pointer against concurrent reads from the worker.
    QMutexLocker locker(&m_stateMutex);
    m_repoName = name;
}

#include "syncengine.h"
#include "svn/svnclient.h"
#include "sync/syncrecordservice.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QThread>

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

void SyncEngine::startSync(const QString &repoName, const QString &localPath, const QString &remoteUrl,
                           const QString &username, const QString &password)
{
    m_repoName = repoName;
    m_localPath = localPath;
    m_remoteUrl = remoteUrl;
    m_username = username;
    m_password = password;
    m_syncing = true;

    if (m_watcher) {
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }

    m_watcher = new QFileSystemWatcher(this);
    if (QDir(m_localPath).exists()) {
        m_watcher->addPath(m_localPath);
    }
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &SyncEngine::onFileChanged);

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
    return m_syncing ? "running" : "stopped";
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
    qDebug() << "[SyncEngine] Debounce timer fired, committing pending files";
    retryPending();
    emit filesChanged();
}

void SyncEngine::onPollTimer()
{
    if (!m_syncing || m_localPath.isEmpty()) return;
    qDebug() << "[SyncEngine] Poll timer fired";
    pollServer();
}

void SyncEngine::onFullSyncTimer()
{
    if (!m_syncing || m_localPath.isEmpty()) return;
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

    qDebug() << "[SyncEngine] Full scan done";
}

void SyncEngine::commitFile(const QString &filePath)
{
    if (!m_svnClient || filePath.isEmpty()) return;

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

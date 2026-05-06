#include "syncengine.h"
#include <QDebug>
#include <QDir>

SyncEngine::SyncEngine(QObject *parent)
    : QObject(parent)
    , m_syncing(false)
{}

SyncEngine::~SyncEngine()
{
    stopSync();
}

void SyncEngine::startSync(const QString &localPath, const QString &remotePath)
{
    m_localPath = localPath;
    m_remotePath = remotePath;
    m_syncing = true;

    // 监听文件变化
    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(localPath);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &SyncEngine::onFileChanged);

    emit syncStarted();
    scanLocalChanges(localPath);
    emit syncFinished();
}

void SyncEngine::stopSync()
{
    m_syncing = false;
    if (m_watcher) {
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }
}

QString SyncEngine::status() const
{
    return m_syncing ? "running" : "stopped";
}

void SyncEngine::onFileChanged(const QString &path)
{
    if (!m_syncing) return;
    emit syncProgress(path);
}

void SyncEngine::scanLocalChanges(const QString &path)
{
    QDir dir(path);
    QFileInfoList infos = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : infos) {
        if (info.isDir()) {
            scanLocalChanges(info.filePath());
        } else {
            emit syncProgress(info.filePath());
        }
    }
}

void SyncEngine::uploadChanges()
{
    // TODO: 实现 SVN add/commit 逻辑
}

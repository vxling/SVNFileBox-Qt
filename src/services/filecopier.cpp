#include "filecopier.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtConcurrent>
#include <QDebug>

FileCopier::FileCopier(QObject *parent) : QObject(parent) {}

FileCopier::~FileCopier()
{
    cancel();
    if (m_watcher) {
        m_watcher->waitForFinished();
        delete m_watcher;
    }
}

void FileCopier::copyPlanAsync(const FileCopyPlan &plan, const QStringList &svnAddPaths)
{
    if (m_running.loadAcquire()) {
        CopyResult r;
        r.errorMessage = "FileCopier is already running";
        emit copyCompleted(r);
        return;
    }
    if (plan.isSameLocation) {
        CopyResult r;
        r.errorMessage = "Source and destination are the same location";
        emit copyCompleted(r);
        return;
    }
    if (plan.items.isEmpty()) {
        CopyResult r;
        emit copyCompleted(r);
        return;
    }

    m_cancelFlag.storeRelease(0);
    m_running.storeRelease(1);

    const QAtomicInt *cancelFlag = &m_cancelFlag;

    // Run copy in background thread; emit progress via signal-slot (queued)
    QFuture<CopyResult> future = QtConcurrent::run([this, plan, svnAddPaths, cancelFlag]() {
        return runCopy(plan, svnAddPaths, cancelFlag);
    });

    if (m_watcher) {
        m_watcher->disconnect();
        delete m_watcher;
    }
    m_watcher = new QFutureWatcher<CopyResult>(this);
    connect(m_watcher, &QFutureWatcher<CopyResult>::finished, this, [this]() {
        CopyResult r = m_watcher->result();
        m_running.storeRelease(0);
        emit copyCompleted(r);
    });
    m_watcher->setFuture(future);
}

void FileCopier::cancel()
{
    m_cancelFlag.storeRelease(1);
}

CopyResult FileCopier::runCopy(const FileCopyPlan &plan, const QStringList &svnAddPaths,
                                const QAtomicInt *cancelFlag)
{
    CopyResult result;
    result.svnAddedPaths = svnAddPaths;

    // Count total files (not dirs) for progress
    int totalFiles = 0;
    for (const FileCopyItem &it : plan.items)
        if (it.itemType == FileCopyItem::File) ++totalFiles;

    int fileIndex = 0;
    qint64 bytesCopied = 0;

    // First pass: create all directories
    for (const FileCopyItem &it : plan.items) {
        if (cancelFlag->loadAcquire()) {
            result.wasCancelled = true;
            return result;
        }
        if (it.itemType == FileCopyItem::Directory) {
            QDir().mkpath(it.destPath);
        }
    }

    // Second pass: copy files
    for (const FileCopyItem &it : plan.items) {
        if (cancelFlag->loadAcquire()) {
            result.wasCancelled = true;
            return result;
        }
        if (it.itemType != FileCopyItem::File) continue;

        // Skip if source and dest are the same
        if (QFileInfo(it.sourcePath).absoluteFilePath() == QFileInfo(it.destPath).absoluteFilePath()) {
            ++result.skippedCount;
            continue;
        }

        bool destExists = QFile::exists(it.destPath);
        if (destExists) {
            // Overwrite: remove first
            QFile::remove(it.destPath);
            ++result.overwrittenCount;
        }

        qint64 copied = 0;
        bool ok = streamCopyFile(it.sourcePath, it.destPath, &copied, cancelFlag);
        if (!ok) {
            if (cancelFlag->loadAcquire()) {
                result.wasCancelled = true;
                return result;
            }
            ++result.skippedCount;
            continue;
        }
        bytesCopied += copied;
        ++result.copiedCount;
        ++fileIndex;

        // Emit progress
        CopyProgress p;
        p.currentFile = it.destPath;
        p.currentIndex = fileIndex;
        p.totalCount = totalFiles;
        p.bytesCopied = bytesCopied;
        p.totalBytes = plan.totalBytes;
        emit const_cast<FileCopier*>(this)->copyProgress(p);
    }

    return result;
}

bool FileCopier::streamCopyFile(const QString &src, const QString &dst,
                                 qint64 *bytesCopied, const QAtomicInt *cancelFlag)
{
    QFile in(src);
    QFile out(dst);

    if (!in.open(QIODevice::ReadOnly)) return false;
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

    // 80 KB buffer (matches WPF FileCopier)
    constexpr qint64 kBufSize = 80 * 1024;
    QByteArray buffer;

    while (!in.atEnd()) {
        if (cancelFlag->loadAcquire()) {
            in.close();
            out.close();
            QFile::remove(dst);
            return false;
        }
        buffer = in.read(kBufSize);
        if (buffer.isEmpty()) break;
        qint64 written = out.write(buffer);
        if (written != buffer.size()) {
            in.close();
            out.close();
            QFile::remove(dst);
            return false;
        }
        *bytesCopied += written;
    }

    in.close();
    out.close();
    return true;
}

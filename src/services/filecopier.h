#ifndef FILECOPIER_H
#define FILECOPIER_H

#include "fileanalyzer.h"
#include <QObject>
#include <QFutureWatcher>
#include <QAtomicInt>

// ── CopyResult ──────────────────────────────────────────────────────────
// Aggregated outcome of a copy plan. Matches WPF CopyResult.
struct CopyResult
{
    int copiedCount = 0;       // files successfully copied
    int skippedCount = 0;      // files skipped (already existed, same-location, etc.)
    int overwrittenCount = 0;  // files that overwrote an existing dest
    QStringList svnAddedPaths; // paths that were svn-add'd after copy
    bool wasCancelled = false;
    QString errorMessage;      // aggregated, non-empty if major failure

    bool ok() const { return errorMessage.isEmpty() && !wasCancelled; }
};

// ── CopyProgress ────────────────────────────────────────────────────────
// Per-step progress, emitted on UI thread. Mirrors WPF IProgress<CopyProgress>.
struct CopyProgress
{
    QString currentFile;       // absolute path of the file being copied
    int currentIndex = 0;      // 0-based, file currently being copied
    int totalCount = 0;        // total number of files
    qint64 bytesCopied = 0;    // running total
    qint64 totalBytes = 0;     // plan totalBytes

    int percent() const
    {
        if (totalBytes <= 0) return 0;
        return static_cast<int>((bytesCopied * 100) / totalBytes);
    }
};

// ── FileCopier (QObject) ─────────────────────────────────────────────────
// Two-phase file copy (matches WPF FileCopier):
//   1. analyze() → FileCopyPlan (in FileAnalyzer, sync)
//   2. copyPlanAsync(plan) → spawns QFuture, emits progress + completed
//
// Cancellation: cancel() sets atomic flag, watcher is interrupted.
// Streaming: 80 KB buffered async copy for large files (vs QFile::copy which
// loads entire file into memory).
class FileCopier : public QObject
{
    Q_OBJECT

public:
    explicit FileCopier(QObject *parent = nullptr);
    ~FileCopier() override;

    // Returns immediately; result delivered via copyCompleted/CopyProgress
    // via signals. Safe to call from any thread.
    void copyPlanAsync(const FileCopyPlan &plan, const QStringList &svnAddPaths);

    // Cancel an in-flight copy. Idempotent.
    void cancel();

    bool isRunning() const { return m_running.loadAcquire(); }

signals:
    // Emitted periodically as the copy progresses (UI thread).
    void copyProgress(const CopyProgress &progress);

    // Emitted exactly once per copyPlanAsync call, with aggregated result.
    void copyCompleted(const CopyResult &result);

private:
    // Worker function — runs on QFuture thread.
    CopyResult runCopy(const FileCopyPlan &plan, const QStringList &svnAddPaths,
                       const QAtomicInt *cancelFlag);

    // Single-file stream copy with 80KB buffer. Returns true on success.
    static bool streamCopyFile(const QString &src, const QString &dst,
                               qint64 *bytesCopied, const QAtomicInt *cancelFlag);

    QFutureWatcher<CopyResult> *m_watcher = nullptr;
    QAtomicInt m_cancelFlag = 0;
    QAtomicInt m_running = 0;
};

#endif // FILECOPIER_H

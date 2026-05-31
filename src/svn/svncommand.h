#pragma once
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>

namespace SVNFileBox {

// ── All SVN commands ──────────────────────────────────────────
enum class SvnCommand {
    // ReadOnly: immediate execution, no queue
    Info,
    Status,
    GetRevision,
    GetHeadRevision,
    GetConflictedFiles,
    GetLastChangedTime,
    IsVersioned,
    IsValidWorkingCopy,
    TestConnection,
    GetServerUpdatePaths,

    // LocalWrite: enqueued, background, drained before HeavyWrite
    Add,
    Delete,
    Move,
    Revert,
    Resolve,
    BreakLock,

    // HeavyWrite: enqueued, one-at-a-time, after all LocalWrite drain
    Commit,
    Update,
    Checkout,
};

// ── Category determines execution path ──────────────────────────
enum class SvnCommandCategory {
    ReadOnly,    // synchronous on caller's thread
    LocalWrite,  // enqueued, background, small-loop drained before HeavyWrite
    HeavyWrite,  // enqueued, background, one-per-iteration
};

// ── Result returned directly for ReadOnly commands (via Task/Future) ──
struct SvnQueryResult {
    bool success = false;
    QString value;
    QString error;

    static SvnQueryResult Ok(const QString &v) { return {true, v, QString()}; }
    static SvnQueryResult Fail(const QString &e) { return {false, QString(), e}; }
};

// ── Result delivered via signal for background commands ──────────
struct SvnCommandResult {
    SvnCommand command;
    QString path;
    bool success = false;
    QString error;
    int revision = -1;
    QString completedAt; // ISO 8601

    static SvnCommandResult Ok(SvnCommand cmd, const QString &path, int rev = -1) {
        return {cmd, path, true, QString(), rev, QDateTime::currentDateTime().toString(Qt::ISODate)};
    }
    static SvnCommandResult Fail(SvnCommand cmd, const QString &path, const QString &err) {
        return {cmd, path, false, err, -1, QDateTime::currentDateTime().toString(Qt::ISODate)};
    }
};

// ── Pending command item queued to worker loop ──────────────────
struct SvnCommandItem {
    SvnCommand command;
    QString path;
    QString fromPath;      // for Move
    QString message;       // for Commit
    QString repoUrl;       // for Checkout
    QString username;
    QString password;
    QStringList updatePaths; // for Update (sub-paths)
    QString accept;          // for Resolve (e.g. "working", " theirs-full")

    // Factory
    static SvnCommandItem make(
        SvnCommand cmd, const QString &path,
        const QString &fromPath = QString(),
        const QString &message = QString(),
        const QString &repoUrl = QString(),
        const QString &username = QString(),
        const QString &password = QString(),
        const QStringList &updatePaths = QStringList(),
        const QString &accept = QString())
    {
        return {cmd, path, fromPath, message, repoUrl, username, password, updatePaths, accept};
    }
};

// ── Category lookup table (built at startup) ────────────────────
SvnCommandCategory commandCategory(SvnCommand cmd);

} // namespace SVNFileBox
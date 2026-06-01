#ifndef SVNCLIENT_H
#define SVNCLIENT_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QElapsedTimer>
#include <QProcess>

namespace SVNFileBox { class SvnCommandExecutor; }

class SVNClient : public QObject
{
    Q_OBJECT
public:
    enum class ErrorLevel { Success, Warning, Error };
    Q_ENUM(ErrorLevel)

public:
    explicit SVNClient(QObject *parent = nullptr);
    ~SVNClient() = default;

    Q_INVOKABLE QStringList list(const QString &path);
    Q_INVOKABLE bool add(const QString &path);
    Q_INVOKABLE bool commit(const QString &path, const QString &message);
    Q_INVOKABLE bool update(const QString &path);
    Q_INVOKABLE bool remove(const QString &path);
    Q_INVOKABLE bool mkdir(const QString &path);
    Q_INVOKABLE bool move(const QString &src, const QString &dst);
    Q_INVOKABLE QString getInfo(const QString &path);
    Q_INVOKABLE QString getStatusString(const QString &path);
    Q_INVOKABLE int getWorkingCopyRevision(const QString &path);
    Q_INVOKABLE int getHeadRevision(const QString &url);
    Q_INVOKABLE int getHeadRevision(const QString &url, const QString &username, const QString &password);
    Q_INVOKABLE bool revert(const QString &path, bool recursive = true);
    Q_INVOKABLE bool cleanup(const QString &path);
    // Force-break stale working-copy locks left by a crashed svn process.
    // Mirrors WPF SvnService.BreakWriteLockAsync.
    Q_INVOKABLE bool breakWriteLock(const QString &path);
    Q_INVOKABLE bool unlock(const QString &path);
    Q_INVOKABLE bool checkout(const QString &url, const QString &localPath,
                               const QString &username = "", const QString &password = "");
    Q_INVOKABLE bool isValidWorkingCopy(const QString &path);
    Q_INVOKABLE QStringList getConflictedFiles(const QString &path);
    Q_INVOKABLE bool resolveConflict(const QString &path, const QString &accept);
    Q_INVOKABLE bool copyFileOrFolder(const QString &src, const QString &dest);

    // Extended read-only API (used by SvnCommandExecutor)
    Q_INVOKABLE QString getRepoUrl(const QString &path);
    Q_INVOKABLE QVariantMap getStatus(const QString &path, bool depth = false);
    Q_INVOKABLE QString getLastChangedTime(const QString &path);
    Q_INVOKABLE bool isVersioned(const QString &path);
    Q_INVOKABLE bool hasIncompleteWorkingCopy(const QString &path);
    Q_INVOKABLE bool testConnection(const QString &url, const QString &username, const QString &password);
    Q_INVOKABLE QStringList getServerUpdatePaths(const QString &path);
    // Lightweight credential probe: runs `svn info <repoUrl>` and returns true
    // if the server replied without auth errors. Mirrors WPF IsCredentialValid.
    // Uses HEAD_REV_TTL_MS cache.
    Q_INVOKABLE bool isCredentialValid(const QString &repoUrl);
    // Return last cached head revision for url, or -1 if unknown.
    Q_INVOKABLE int cachedHeadRevision(const QString &url) const;

signals:
    void commandFinished(const QString &output);
    void commandError(const QString &error);
    void commandWarning(const QString &warning);
    // Per-file transfer event. Emitted by `update` with each file's transfer
    // result (filename, transferredBytes, totalBytes or -1 if unknown).
    // Mirrors WPF SvnService.FileTransferActivity. Listeners (QML status bar)
    // can show per-file progress.
    void fileTransferActivity(const QString &filePath, qint64 bytesTransferred, qint64 bytesTotal);

    friend class SVNFileBox::SvnCommandExecutor;

private:
    static constexpr int DEFAULT_TIMEOUT_MS = 60'000;       // 60s for read ops
    static constexpr int HEAVYWRITE_TIMEOUT_MS = 600'000;   // 600s safety ceiling for HeavyWrite
    static constexpr int SAFETY_TIMEOUT_MS = 600'000;       // absolute max for any SVN call

    QString runSvn(const QStringList &args, const QString &workDir = QString(), int timeoutMs = DEFAULT_TIMEOUT_MS);
    bool runSvnBool(const QStringList &args, const QString &workDir = QString(), int timeoutMs = DEFAULT_TIMEOUT_MS);
    ErrorLevel runSvnLevel(const QStringList &args, const QString &workDir = QString(), int timeoutMs = DEFAULT_TIMEOUT_MS);
    bool runSvnTimed(const QStringList &args, const QString &workDir, int timeoutMs, QString *output = nullptr);

    // Head revision cache: {url -> {revision, timestamp}}
    struct HeadRevEntry {
        int revision = -1;
        QElapsedTimer timestamp;
    };
    mutable QHash<QString, HeadRevEntry> m_headRevCache;
    mutable QMutex m_headRevCacheMutex;
};

#endif // SVNCLIENT_H

#ifndef SVNCLIENT_H
#define SVNCLIENT_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>
#include <QtCore/QMutex>
#include <QtCore/QElapsedTimer>

// Forward libsvn types — included here only to allow the Pimpl to hold them.
// All libsvn symbols stay internal to svnclient.cpp; this header never re-exports them.
struct svn_client_ctx_t;
struct svn_commit_info_t;
struct svn_wc_notify_t;
struct apr_pool_t;

namespace SVNFileBox { class SvnCommandExecutor; }

class SVNClient : public QObject
{
    Q_OBJECT
public:
    enum class ErrorLevel { Success, Warning, Error };
    Q_ENUM(ErrorLevel)

    explicit SVNClient(QObject *parent = nullptr);
    ~SVNClient() override;

    SVNClient(const SVNClient&) = delete;
    SVNClient& operator=(const SVNClient&) = delete;

    // ── Per-client configuration (set before any operations) ─────
    void setUsername(const QString &u);
    void setPassword(const QString &p);
    void setConfigDir(const QString &dir);
    void setTrustedMode(bool on);

    // ── SVN write operations ────────────────────────────────────
    Q_INVOKABLE bool add(const QString &path);
    Q_INVOKABLE bool remove(const QString &path);
    Q_INVOKABLE bool commit(const QString &path, const QString &message);
    Q_INVOKABLE bool update(const QString &path);
    Q_INVOKABLE bool mkdir(const QString &path);
    Q_INVOKABLE bool move(const QString &src, const QString &dst);
    Q_INVOKABLE bool revert(const QString &path, bool recursive = true);
    Q_INVOKABLE bool cleanup(const QString &path);
    Q_INVOKABLE bool unlock(const QString &path);
    Q_INVOKABLE bool checkout(const QString &url, const QString &localPath);
    Q_INVOKABLE bool resolveConflict(const QString &path, const QString &accept);
    Q_INVOKABLE bool copyFileOrFolder(const QString &src, const QString &dest);
    Q_INVOKABLE bool breakWriteLock(const QString &path);

    // ── SVN read operations ─────────────────────────────────────
    Q_INVOKABLE QString getRepoUrl(const QString &path);
    Q_INVOKABLE QVariantMap getInfo(const QString &path);
    Q_INVOKABLE QString getStatusString(const QString &path);
    Q_INVOKABLE QString getLastChangedTime(const QString &path);
    Q_INVOKABLE int getWorkingCopyRevision(const QString &path);
    Q_INVOKABLE int getHeadRevision(const QString &url);
    Q_INVOKABLE bool isVersioned(const QString &path);
    Q_INVOKABLE bool isValidWorkingCopy(const QString &path);
    Q_INVOKABLE bool hasIncompleteWorkingCopy(const QString &path);
    Q_INVOKABLE bool testConnection(const QString &url);
    Q_INVOKABLE bool isCredentialValid(const QString &url);
    Q_INVOKABLE int cachedHeadRevision(const QString &url) const;
    Q_INVOKABLE QStringList list(const QString &path);
    Q_INVOKABLE QStringList getConflictedFiles(const QString &path);
    Q_INVOKABLE QStringList getServerUpdatePaths(const QString &path);
    Q_INVOKABLE QVariantMap getStatus(const QString &path, bool depth = false);
    Q_INVOKABLE QVariantMap batchGetStatus(const QString &dirPath);

signals:
    void commandFinished(const QString &output);
    void commandError(const QString &error);
    void commandWarning(const QString &warning);
    // Per-file transfer event emitted during update with each file's
    // transfer result (filename, transferredBytes, totalBytes or -1).
    void fileTransferActivity(const QString &filePath, qint64 transferred, qint64 total);

    friend class SVNFileBox::SvnCommandExecutor;

private:
    static constexpr int DEFAULT_TIMEOUT_MS = 60'000;
    static constexpr int HEAVYWRITE_TIMEOUT_MS = 600'000;
    static constexpr int SAFETY_TIMEOUT_MS = 600'000;

    // Pimpl — complete definition in svnclient.cpp; this header only holds the pointer.
    // All libsvn types are invisible to every other translation unit.
    struct Private;
    Private *d;
};

#endif // SVNCLIENT_H
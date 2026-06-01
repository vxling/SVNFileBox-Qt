#ifndef SVNCLIENT_H
#define SVNCLIENT_H

#include <QObject>
#include <QString>
#include <QStringList>
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
    // Clear SVN's auth cache for a specific URL (or all if url empty).
    // After this, the next SVN command will prompt for credentials again.
    // Mirrors WPF ClearAuthenticationCache().
    Q_INVOKABLE bool clearAuthCache(const QString &url = QString());

signals:
    void commandFinished(const QString &output);
    void commandError(const QString &error);
    void commandWarning(const QString &warning);

    friend class SVNFileBox::SvnCommandExecutor;

private:
    static constexpr int DEFAULT_TIMEOUT_MS = 60'000;       // 60s for read ops
    static constexpr int HEAVYWRITE_TIMEOUT_MS = 600'000;   // 600s safety ceiling for HeavyWrite
    static constexpr int SAFETY_TIMEOUT_MS = 600'000;       // absolute max for any SVN call

    QString runSvn(const QStringList &args, const QString &workDir = QString(), int timeoutMs = DEFAULT_TIMEOUT_MS);
    bool runSvnBool(const QStringList &args, const QString &workDir = QString(), int timeoutMs = DEFAULT_TIMEOUT_MS);
    ErrorLevel runSvnLevel(const QStringList &args, const QString &workDir = QString(), int timeoutMs = DEFAULT_TIMEOUT_MS);
    bool runSvnTimed(const QStringList &args, const QString &workDir, int timeoutMs, QString *output = nullptr);
};

#endif // SVNCLIENT_H

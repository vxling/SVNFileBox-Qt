#include "svnclient.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QRegularExpression>

SVNClient::SVNClient(QObject *parent) : QObject(parent) {}

QStringList SVNClient::list(const QString &path)
{
    QStringList args = {"list", "--non-interactive", "--trust-server-cert", "--xml", path};
    QString output = runSvn(args);
    QStringList result;
    QXmlStreamReader xml(output);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("entry")) {
            QString name = xml.attributes().value("name").toString();
            if (!name.isEmpty())
                result.append(name);
        }
    }
    return result;
}

bool SVNClient::add(const QString &path)
{
    return runSvnBool({"add", "--non-interactive", "--trust-server-cert", path});
}

bool SVNClient::commit(const QString &path, const QString &message)
{
    QStringList args = {"commit", "-m", message, "--non-interactive", "--trust-server-cert", path};
    return runSvnBool(args);
}

bool SVNClient::update(const QString &path)
{
    return runSvnBool({"update", "--non-interactive", "--trust-server-cert", path});
}

bool SVNClient::remove(const QString &path)
{
    return runSvnBool({"remove", "--non-interactive", "--trust-server-cert", path});
}

bool SVNClient::mkdir(const QString &path)
{
    return runSvnBool({"mkdir", "--non-interactive", "--trust-server-cert", path});
}

bool SVNClient::move(const QString &src, const QString &dst)
{
    return runSvnBool({"move", "--non-interactive", "--trust-server-cert", src, dst});
}

QString SVNClient::getInfo(const QString &path)
{
    QStringList args = {"info", "--non-interactive", "--trust-server-cert", "--xml", path};
    return runSvn(args);
}

QString SVNClient::getStatusString(const QString &path)
{
    QStringList args = {"status", "--non-interactive", "--trust-server-cert", "--xml", path};
    QString output = runSvn(args);
    QXmlStreamReader xml(output);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("wc-status")) {
            QString item = xml.attributes().value("item").toString();
            if (!item.isEmpty() && item != "normal") {
                // Title-case: first char upper, rest lower
                if (!item.isEmpty()) {
                    item[0] = item[0].toUpper();
                    for (int i = 1; i < item.size(); ++i)
                        item[i] = item[i].toLower();
                }
                return item;
            }
        }
    }
    return "Normal";
}

bool SVNClient::runSvnTimed(const QStringList &args, const QString &workDir, int timeoutMs, QString *output)
{
    QProcess p;
    if (!workDir.isEmpty())
        p.setWorkingDirectory(workDir);
    p.start("svn", args);
    if (!p.waitForStarted(5000)) {
        qWarning() << "[SVNClient] Failed to start svn process:" << args;
        return false;
    }
    bool finished = p.waitForFinished(timeoutMs);
    if (!finished) {
        qWarning() << "[SVNClient] SVN timed out after" << timeoutMs << "ms:" << args;
        p.kill();
        p.waitForFinished(2000);
        return false;
    }
    if (output)
        *output = QString::fromLocal8Bit(p.readAllStandardOutput());
    QString errorOutput = QString::fromLocal8Bit(p.readAllStandardError());
    int exitCode = p.exitCode();

    // Detect authentication failures (SVN error codes E170001-E170014)
    if (exitCode != 0) {
        QString err = errorOutput.trimmed();
        if (err.contains(QStringLiteral("E170001")) || err.contains(QStringLiteral("Authentication failed")) ||
            err.contains(QStringLiteral("E230001")) || err.contains(QStringLiteral("Server SSL certificate")) ||
            err.contains(QStringLiteral("credential"))) {
            qWarning() << "[SVNClient] Auth/ssl error detected, exitCode=" << exitCode << ":" << err;
            emit commandError(QStringLiteral("auth_error:") + err);
            return false;
        }
    }
    return exitCode == 0;
}

QString SVNClient::runSvn(const QStringList &args, const QString &workDir, int timeoutMs)
{
    int cap = qMin(timeoutMs, SAFETY_TIMEOUT_MS);
    QString output;
    runSvnTimed(args, workDir, cap, &output);
    return output;
}

bool SVNClient::runSvnBool(const QStringList &args, const QString &workDir, int timeoutMs)
{
    int cap = qMin(timeoutMs, SAFETY_TIMEOUT_MS);
    return runSvnTimed(args, workDir, cap);
}

SVNClient::ErrorLevel SVNClient::runSvnLevel(const QStringList &args, const QString &workDir, int timeoutMs)
{
    int cap = qMin(timeoutMs, SAFETY_TIMEOUT_MS);
    QProcess p;
    if (!workDir.isEmpty())
        p.setWorkingDirectory(workDir);
    p.start("svn", args);
    if (!p.waitForStarted(5000)) {
        qWarning() << "[SVNClient] Failed to start svn process:" << args;
        return ErrorLevel::Error;
    }
    bool finished = p.waitForFinished(cap);
    if (!finished) {
        qWarning() << "[SVNClient] SVN timed out after" << cap << "ms:" << args;
        p.kill();
        p.waitForFinished(2000);
        return ErrorLevel::Error;
    }
    QString error = QString::fromLocal8Bit(p.readAllStandardError());
    QString output = QString::fromLocal8Bit(p.readAllStandardOutput());
    int exitCode = p.exitCode();

    // Warning-level patterns
    QStringList warningPatterns = {
        "Nothing to commit", "Skipped", "At revision",
        "Updating", "Summary of conflicts", "Revision"
    };
    for (const QString &pat : warningPatterns) {
        if (error.contains(pat) || output.contains(pat)) {
            if (exitCode == 0 || error.contains("At revision")) {
                if (!error.isEmpty())
                    emit commandWarning(error.isEmpty() ? output : error);
                return ErrorLevel::Warning;
            }
        }
    }

    // Error-level patterns
    QStringList errorPatterns = {
        "Authentication required", "authorization failed",
        "Can't create directory", "Permission denied",
        "File not found", "Working copy already locked",
        "Run 'svn cleanup'", "conflict", "Out of date", "Invalid URL"
    };
    for (const QString &pat : errorPatterns) {
        if (error.contains(pat)) {
            qWarning() << "svn error:" << error;
            emit commandError(error);
            return ErrorLevel::Error;
        }
    }

    if (exitCode != 0) {
        if (!error.isEmpty()) {
            qWarning() << "svn error:" << error;
            emit commandError(error);
        }
        return ErrorLevel::Error;
    }

    if (!error.isEmpty()) {
        qWarning() << "svn warning:" << error;
        emit commandWarning(error);
    }
    return ErrorLevel::Success;
}

int SVNClient::getWorkingCopyRevision(const QString &path)
{
    QStringList args = {"info", "--non-interactive", "--trust-server-cert", "--xml", path};
    QString output = runSvn(args);
    QXmlStreamReader xml(output);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("entry")) {
            QString rev = xml.attributes().value("revision").toString();
            if (!rev.isEmpty())
                return rev.toInt();
        }
    }
    return -1;
}

int SVNClient::getHeadRevision(const QString &url)
{
    // 30s TTL cache to avoid hitting server for every status check.
    // Mirrors WPF SvnService.GetHeadRevision cache.
    static constexpr int HEAD_REV_TTL_MS = 30'000;
    QMutexLocker locker(&m_headRevCacheMutex);
    auto it = m_headRevCache.find(url);
    if (it != m_headRevCache.end()) {
        if (it.value().timestamp.elapsed() < HEAD_REV_TTL_MS) {
            return it.value().revision;
        }
    }
    locker.unlock();

    QStringList args = {"info", "--non-interactive", "-r", "HEAD", "--trust-server-cert", "--xml", url};
    QString output = runSvn(args);
    int rev = -1;
    QXmlStreamReader xml(output);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("entry")) {
            QString r = xml.attributes().value("revision").toString();
            if (!r.isEmpty()) {
                rev = r.toInt();
                break;
            }
        }
    }

    // Cache result (even -1, to avoid retries on persistent failure within TTL)
    locker.relock();
    m_headRevCache[url] = { rev, QElapsedTimer() };
    m_headRevCache[url].timestamp.start();
    return rev;
}

int SVNClient::cachedHeadRevision(const QString &url) const
{
    QMutexLocker locker(&m_headRevCacheMutex);
    auto it = m_headRevCache.find(url);
    if (it != m_headRevCache.end())
        return it.value().revision;
    return -1;
}

bool SVNClient::isCredentialValid(const QString &repoUrl)
{
    // Lightweight probe: use `svn info` with a short timeout. If it succeeds
    // AND no "authorization failed" appears in stderr, creds are good.
    // Reuses the head-revision cache to avoid duplicate network calls.
    int rev = getHeadRevision(repoUrl);
    if (rev > 0) return true;

    // Fallback: explicit info call with short timeout
    QStringList args = {"info", "--non-interactive", "--trust-server-cert",
                        "--xml", repoUrl};
    QString output = runSvn(args, QString(), 10'000);
    if (output.isEmpty()) return false;
    if (output.contains(QStringLiteral("authorization failed"), Qt::CaseInsensitive))
        return false;
    return output.contains(QStringLiteral("<entry"));
}

int SVNClient::getHeadRevision(const QString &url, const QString &username, const QString &password)
{
    QStringList args = {"info", "--non-interactive", "-r", "HEAD", "--trust-server-cert", "--xml", url};
    if (!username.isEmpty()) {
        args.append("--username"); args.append(username);
        args.append("--password"); args.append(password);
    }
    QString output = runSvn(args);
    QXmlStreamReader xml(output);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("entry")) {
            QString rev = xml.attributes().value("revision").toString();
            if (!rev.isEmpty())
                return rev.toInt();
        }
    }
    return -1;
}

bool SVNClient::revert(const QString &path, bool recursive)
{
    QStringList args = {"revert", "--non-interactive", "--trust-server-cert", path};
    if (recursive) args.insert(2, "--recursive");
    return runSvnBool(args);
}

bool SVNClient::cleanup(const QString &path)
{
    return runSvnBool({"cleanup", "--non-interactive", "--trust-server-cert", path});
}

// Force-break stale working copy locks left by crashed svn processes.
// Mirrors WPF SvnService.BreakWriteLockAsync. `svn cleanup --break-wait`
// aborts any currently-waiting svn client on this working copy and clears
// the lock files.
bool SVNClient::breakWriteLock(const QString &path)
{
    if (path.isEmpty()) return false;
    return runSvnBool({"cleanup", "--non-interactive", "--trust-server-cert",
                       "--break-wait", path});
}

bool SVNClient::unlock(const QString &path)
{
    return runSvnBool({"unlock", "--non-interactive", "--trust-server-cert", path});
}

bool SVNClient::checkout(const QString &url, const QString &localPath,
                          const QString &username, const QString &password) {
    QStringList args = {"checkout", "--non-interactive", "--trust-server-cert"};
    if (!username.isEmpty()) {
        args += {"--username", username};
        if (!password.isEmpty()) {
            args += {"--password", password};
        }
    }
    args += {url, localPath};
    return runSvnBool(args);
}

bool SVNClient::isValidWorkingCopy(const QString &path)
{
    if (!QDir(path).exists()) return false;
    // SVN 1.6+: .svn/entries 文件
    // SVN 1.14+: .svn/wc.db (SQLite)
    return QFile::exists(path + "/.svn/entries") || QFile::exists(path + "/.svn/wc.db");
}

QStringList SVNClient::getConflictedFiles(const QString &path)
{
    QStringList args = {"status", "--non-interactive", "--trust-server-cert", "--xml", path};
    QString output = runSvn(args);
    QStringList conflicted;
    QXmlStreamReader xml(output);
    QString curPath;
    bool isConflicted = false;
    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType tok = xml.readNext();
        if (tok == QXmlStreamReader::StartElement) {
            QStringView name = xml.name();
            if (name == QStringLiteral("entry")) {
                curPath = xml.attributes().value("path").toString();
                isConflicted = false;
            } else if (name == QStringLiteral("wc-status") && xml.attributes().value("item") == QStringLiteral("conflicted")) {
                isConflicted = true;
            }
        } else if (tok == QXmlStreamReader::EndElement && xml.name() == QStringLiteral("entry")) {
            if (isConflicted && !curPath.isEmpty())
                conflicted.append(curPath);
        }
    }
    return conflicted;
}

bool SVNClient::resolveConflict(const QString &path, const QString &accept)
{
    // accept: "mine-conflict" (keep local) or "theirs-conflict" (use server)
    QStringList args = {"resolve", "--non-interactive", "--trust-server-cert",
                        "--accept", accept, path};
    return runSvnBool(args);
}

bool SVNClient::copyFileOrFolder(const QString &src, const QString &dest)
{
    QStringList args = {"copy", "--non-interactive", "--trust-server-cert", src, dest};
    return runSvnBool(args);
}

// ── Extended read-only API (used by SvnCommandExecutor) ────────

QString SVNClient::getRepoUrl(const QString &path)
{
    QStringList args = {"info", "--non-interactive", "--trust-server-cert", "--xml", path};
    QString output = runSvn(args);
    QXmlStreamReader xml(output);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("url")) {
            return xml.readElementText();
        }
    }
    return QString();
}

QVariantMap SVNClient::getStatus(const QString &path, bool depth)
{
    QVariantMap result;
    QStringList args = {"status", "--non-interactive", "--trust-server-cert", "--xml"};
    if (depth) args.append("--depth=infinity");
    args.append(path);
    QString output = runSvn(args);
    QXmlStreamReader xml(output);
    QString curPath;
    QString curItem;
    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType tok = xml.readNext();
        if (tok == QXmlStreamReader::StartElement) {
            QStringView name = xml.name();
            if (name == QStringLiteral("entry")) {
                curPath = xml.attributes().value("path").toString();
                curItem.clear();
            } else if (name == QStringLiteral("wc-status")) {
                QString item = xml.attributes().value("item").toString();
                QString prop = xml.attributes().value("props").toString();
                if (!item.isEmpty())
                    curItem = item;
                // Detect tree conflict: XML shows tree-conflicted="true" on wc-status
                if (xml.attributes().value("tree-conflicted") == QStringLiteral("true")) {
                    curItem = QStringLiteral("treeconflicted");
                }
            }
        } else if (tok == QXmlStreamReader::EndElement && xml.name() == QStringLiteral("entry")) {
            if (!curPath.isEmpty() && !curItem.isEmpty())
                result[curPath] = curItem;
        }
    }
    return result;
}

QString SVNClient::getLastChangedTime(const QString &path)
{
    QStringList args = {"info", "--non-interactive", "--trust-server-cert", "--xml", path};
    QString output = runSvn(args);
    QXmlStreamReader xml(output);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("date")) {
            return xml.readElementText();
        }
    }
    return QString();
}

bool SVNClient::isVersioned(const QString &path)
{
    return isValidWorkingCopy(path) || QFile::exists(QFileInfo(path).dir().absolutePath() + "/.svn");
}

bool SVNClient::hasIncompleteWorkingCopy(const QString &path)
{
    QStringList args = {"status", "--non-interactive", "--trust-server-cert", "--xml", "--depth=infinity", path};
    QString output = runSvn(args);
    QXmlStreamReader xml(output);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("wc-status")) {
            // SVN reports "incomplete" as the 'item' or 'depth' attribute when WC is corrupted
            QString item = xml.attributes().value("item").toString();
            if (item == QStringLiteral("incomplete"))
                return true;
        }
    }
    return false;
}

bool SVNClient::testConnection(const QString &url, const QString &username, const QString &password)
{
    QStringList args = {"info", "--non-interactive", "--trust-server-cert"};
    if (!username.isEmpty()) {
        args += {"--username", username};
        if (!password.isEmpty())
            args += {"--password", password};
    }
    args.append(url);
    QString output = runSvn(args);
    return output.contains("Revision:") || output.contains("URL:");
}

QStringList SVNClient::getServerUpdatePaths(const QString &path)
{
    // svn status -u --xml shows incoming changes with '*' marker
    QStringList args = {"status", "--non-interactive", "--trust-server-cert", "--xml", "-u", path};
    QString output = runSvn(args);
    QStringList paths;
    QXmlStreamReader xml(output);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("entry")) {
            // Only include entries that have remote updates (status="hidden" or has 'r'* marker)
            QString filePath = xml.attributes().value("path").toString();
            if (!filePath.isEmpty())
                paths.append(filePath);
        }
    }
    return paths;
}

#include "svnclient.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegularExpression>

SVNClient::SVNClient(QObject *parent) : QObject(parent) {}

QStringList SVNClient::list(const QString &path)
{
    QStringList args = {"list", "--non-interactive", "--trust-server-cert", path};
    QString output = runSvn(args);
    return output.split('\n', Qt::SkipEmptyParts);
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
    return runSvn({"info", "--non-interactive", "--trust-server-cert", path});
}

QString SVNClient::getStatus(const QString &path)
{
    QString output = runSvn({"status", "--non-interactive", "--trust-server-cert", path});
    if (output.isEmpty()) return "Normal";
    // Parse first character of first line
    QChar statusChar = output[0];
    switch (statusChar.unicode()) {
        case 'A': return "Added";
        case 'D': return "Deleted";
        case 'M': return "Modified";
        case 'R': return "Replaced";
        case 'C': return "Conflicted";
        case 'G': return "Merged";
        case 'U': return "Updated";
        case '?': return "Unversioned";
        case '!': return "Missing";
        case '~': return "Obstructed";
        case 'I': return "Ignored";
        default:  return "Normal";
    }
}

QString SVNClient::runSvn(const QStringList &args, const QString &workDir)
{
    QProcess p;
    if (!workDir.isEmpty()) {
        p.setWorkingDirectory(workDir);
    }
    p.start("svn", args);
    p.waitForFinished(-1);
    QString output = QString::fromLocal8Bit(p.readAllStandardOutput());
    QString error = QString::fromLocal8Bit(p.readAllStandardError());
    if (!error.isEmpty()) {
        qWarning() << "svn error:" << error;
        emit commandError(error);
    }
    return output;
}

bool SVNClient::runSvnBool(const QStringList &args, const QString &workDir)
{
    QProcess p;
    if (!workDir.isEmpty()) {
        p.setWorkingDirectory(workDir);
    }
    p.start("svn", args);
    p.waitForFinished(-1);
    return p.exitCode() == 0;
}

int SVNClient::getWorkingCopyRevision(const QString &path)
{
    QString output = runSvn({"info", "--non-interactive", "--trust-server-cert", path});
    QRegularExpression re(R"(^Revision:\s*(\d+))", QRegularExpression::MultilineOption);
    QRegularExpressionMatch m = re.match(output);
    if (m.hasMatch()) return m.captured(1).toInt();
    return -1;
}

int SVNClient::getHeadRevision(const QString &url)
{
    QString output = runSvn({"info", "--non-interactive", "-r", "HEAD", "--trust-server-cert", url});
    QRegularExpression re(R"(^Revision:\s*(\d+))", QRegularExpression::MultilineOption);
    QRegularExpressionMatch m = re.match(output);
    if (m.hasMatch()) return m.captured(1).toInt();
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

bool SVNClient::unlock(const QString &path)
{
    return runSvnBool({"unlock", "--non-interactive", "--trust-server-cert", path});
}

bool SVNClient::checkout(const QString &url, const QString &localPath)
{
    return runSvnBool({"checkout", "--non-interactive", "--trust-server-cert", url, localPath});
}

bool SVNClient::isValidWorkingCopy(const QString &path)
{
    return QDir(path).exists() && QFile::exists(path + "/.svn/entries");
}

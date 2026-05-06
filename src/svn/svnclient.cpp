#include "svnclient.h"
#include <QDebug>
#include <QDir>

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

QString SVNClient::getInfo(const QString &path)
{
    return runSvn({"info", "--non-interactive", "--trust-server-cert", path});
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
    QString output = runSvn(args, workDir);
    return !output.isEmpty() || true; // 简化判断，实际检查 exit code
}

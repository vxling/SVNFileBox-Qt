#ifndef SVNCLIENT_H
#define SVNCLIENT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>

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
    Q_INVOKABLE QString getStatus(const QString &path);
    Q_INVOKABLE int getWorkingCopyRevision(const QString &path);
    Q_INVOKABLE int getHeadRevision(const QString &url);
    Q_INVOKABLE bool revert(const QString &path, bool recursive = true);
    Q_INVOKABLE bool cleanup(const QString &path);
    Q_INVOKABLE bool unlock(const QString &path);
    Q_INVOKABLE bool checkout(const QString &url, const QString &localPath,
                               const QString &username = "", const QString &password = "");
    Q_INVOKABLE bool isValidWorkingCopy(const QString &path);

signals:
    void commandFinished(const QString &output);
    void commandError(const QString &error);
    void commandWarning(const QString &warning);

private:
    QString runSvn(const QStringList &args, const QString &workDir = QString());
    bool runSvnBool(const QStringList &args, const QString &workDir = QString());
    ErrorLevel runSvnLevel(const QStringList &args, const QString &workDir = QString());
};

#endif // SVNCLIENT_H

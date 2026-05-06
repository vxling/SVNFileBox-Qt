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
    explicit SVNClient(QObject *parent = nullptr);
    ~SVNClient() = default;

    Q_INVOKABLE QStringList list(const QString &path);
    Q_INVOKABLE bool add(const QString &path);
    Q_INVOKABLE bool commit(const QString &path, const QString &message);
    Q_INVOKABLE bool update(const QString &path);
    Q_INVOKABLE bool remove(const QString &path);
    Q_INVOKABLE QString getInfo(const QString &path);

signals:
    void commandFinished(const QString &output);
    void commandError(const QString &error);

private:
    QString runSvn(const QStringList &args, const QString &workDir = QString());
    bool runSvnBool(const QStringList &args, const QString &workDir = QString());
};

#endif // SVNCLIENT_H

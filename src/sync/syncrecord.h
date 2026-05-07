#ifndef SYNCRECORD_H
#define SYNCRECORD_H

#include <QObject>
#include <QString>
#include <QDateTime>

class SyncRecord : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(QString repoName READ repoName)
    Q_PROPERTY(QString filePath READ filePath)
    Q_PROPERTY(QString operation READ operation)
    Q_PROPERTY(QString result READ result)
    Q_PROPERTY(QString message READ message)
    Q_PROPERTY(QString timestamp READ timestamp)

public:
    explicit SyncRecord(QObject *parent = nullptr) : QObject(parent) {}
    SyncRecord(const QString &repo, const QString &file, const QString &op,
               const QString &res, const QString &msg, QObject *parent = nullptr)
        : QObject(parent), m_repo(repo), m_file(file), m_op(op), m_res(res), m_msg(msg), m_ts(QDateTime::currentDateTime()) {}

    QString repoName() const { return m_repo; }
    QString filePath() const { return m_file; }
    QString operation() const { return m_op; }
    QString result() const { return m_res; }
    QString message() const { return m_msg; }
    QString timestamp() const { return m_ts.toString("yyyy-MM-dd HH:mm:ss"); }

private:
    QString m_repo;
    QString m_file;
    QString m_op;
    QString m_res;
    QString m_msg;
    QDateTime m_ts;
};

#endif // SYNCRECORD_H

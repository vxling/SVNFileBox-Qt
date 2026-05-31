#ifndef SYNCRECORD_H
#define SYNCRECORD_H

#include <QObject>
#include <QString>
#include <QDateTime>

class SyncRecord : public QObject
{
    Q_OBJECT
    Q_PROPERTY(long id READ id NOTIFY refreshed)
    Q_PROPERTY(QString repoName READ repoName NOTIFY refreshed)
    Q_PROPERTY(QString filePath READ filePath NOTIFY refreshed)
    Q_PROPERTY(QString operation READ operation NOTIFY refreshed)
    Q_PROPERTY(QString result READ result NOTIFY refreshed)
    Q_PROPERTY(QString message READ message NOTIFY refreshed)
    Q_PROPERTY(QString timestamp READ timestamp NOTIFY refreshed)

public:
    explicit SyncRecord(QObject *parent = nullptr) : QObject(parent) {}
    SyncRecord(const QString &repo, const QString &file, const QString &op,
               const QString &res, const QString &msg, QObject *parent = nullptr)
        : QObject(parent), m_repo(repo), m_file(file), m_op(op), m_res(res), m_msg(msg), m_ts(QDateTime::currentDateTime()) {
        emit refreshed();
    }

    QString repoName() const { return m_repo; }
    QString filePath() const { return m_file; }
    QString operation() const { return m_op; }
    QString result() const { return m_res; }
    QString message() const { return m_msg; }
    QString timestamp() const { return m_ts.toString("yyyy-MM-dd HH:mm:ss"); }
    void setTimestamp(const QDateTime &ts) { m_ts = ts; }
    long id() const { return m_id; }
    void setId(long id) { m_id = id; }

signals:
    void refreshed();

private:
    long m_id = 0;
    QString m_repo;
    QString m_file;
    QString m_op;
    QString m_res;
    QString m_msg;
    QDateTime m_ts;
};

#endif // SYNCRECORD_H

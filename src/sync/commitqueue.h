#ifndef COMMITQUEUE_H
#define COMMITQUEUE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMutex>
#include <QMutexLocker>

class CommitQueue : public QObject
{
    Q_OBJECT

public:
    enum CommitOperation {
        OpModify = 0,
        OpAdd,
        OpDelete,
        OpMove
    };
    Q_ENUM(CommitOperation)

    enum CommitStatus {
        StatusPending = 0,
        StatusInProgress,
        StatusCommitted,
        StatusFailed
    };
    Q_ENUM(CommitStatus)

    struct Item {
        QString path;
        QString fromPath;   // for Move
        int operation = OpModify;
        int status = StatusPending;
        qint64 queuedAt = 0; // epoch ms
        int retryCount = 0;
    };

    explicit CommitQueue(QObject *parent = nullptr);
    ~CommitQueue() = default;

    Q_INVOKABLE int count() const;
    Q_INVOKABLE void enqueue(const QString &path, int operation, const QString &fromPath = QString());
    Q_INVOKABLE void enqueueMove(const QString &fromPath, const QString &toPath);
    Q_INVOKABLE QList<Item> resolve();
    Q_INVOKABLE void markInProgress(const QList<Item> &items);
    Q_INVOKABLE void markCommitted(const QList<Item> &items);
    Q_INVOKABLE void markFailed(const QList<Item> &items);
    Q_INVOKABLE QList<Item> getStaleItems(int maxRetries = 3) const;
    Q_INVOKABLE void prune();

    Q_INVOKABLE QString toJson() const;
    Q_INVOKABLE static QString operationName(int op);
    Q_INVOKABLE static QString statusName(int st);

signals:
    void queueChanged();

private:
    void removeCommitted(const QList<Item> &items);

    mutable QMutex m_mutex;
    QList<Item> m_items;
};

#endif // COMMITQUEUE_H

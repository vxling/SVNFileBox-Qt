#include "commitqueue.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

CommitQueue::CommitQueue(QObject *parent)
    : QObject(parent)
{
}

int CommitQueue::count() const
{
    QMutexLocker locker(&m_mutex);
    return m_items.count();
}

void CommitQueue::enqueue(const QString &path, int operation, const QString &fromPath)
{
    if (path.isEmpty()) return;

    Item item;
    item.path = path;
    item.fromPath = fromPath;
    item.operation = operation;
    item.status = StatusPending;
    item.queuedAt = QDateTime::currentMSecsSinceEpoch();
    item.retryCount = 0;

    {
        QMutexLocker locker(&m_mutex);
        m_items.append(item);
    }

    emit queueChanged();
}

void CommitQueue::enqueueMove(const QString &fromPath, const QString &toPath)
{
    enqueue(toPath, OpMove, fromPath);
}

QList<CommitQueue::Item> CommitQueue::resolve()
{
    QMutexLocker locker(&m_mutex);

    QMap<QString, Item> seen;
    for (int i = m_items.size() - 1; i >= 0; --i) {
        const Item &it = m_items[i];
        QString key = it.operation == OpMove ? it.path : it.path;
        if (!seen.contains(key))
            seen[key] = it;
        if (it.operation == OpMove && !seen.contains(it.fromPath))
            seen[it.fromPath] = it;
    }

    QList<Item> resolved = seen.values();

    QList<Item> ordered;
    for (const Item &it : resolved) {
        if (it.operation == OpDelete) ordered.append(it);
    }
    for (const Item &it : resolved) {
        if (it.operation == OpMove) ordered.append(it);
    }
    for (const Item &it : resolved) {
        if (it.operation == OpAdd) ordered.append(it);
    }
    for (const Item &it : resolved) {
        if (it.operation == OpModify) ordered.append(it);
    }

    return ordered;
}

void CommitQueue::markInProgress(const QList<Item> &items)
{
    QMutexLocker locker(&m_mutex);
    for (const Item &it : items) {
        for (Item &mi : m_items) {
            if (mi.path == it.path && mi.queuedAt == it.queuedAt) {
                mi.status = StatusInProgress;
                break;
            }
        }
    }
}

void CommitQueue::markCommitted(const QList<Item> &items)
{
    {
        QMutexLocker locker(&m_mutex);
        removeCommitted(items);
    }
    emit queueChanged();
}

void CommitQueue::markFailed(const QList<Item> &items)
{
    QMutexLocker locker(&m_mutex);
    for (const Item &it : items) {
        for (Item &mi : m_items) {
            if (mi.path == it.path && mi.queuedAt == it.queuedAt) {
                mi.status = StatusFailed;
                mi.retryCount++;
                break;
            }
        }
    }
}

QList<CommitQueue::Item> CommitQueue::getStaleItems(int maxRetries) const
{
    QMutexLocker locker(&m_mutex);
    QList<Item> stale;
    for (const Item &it : m_items) {
        if (it.status == StatusFailed && it.retryCount >= maxRetries)
            stale.append(it);
    }
    return stale;
}

void CommitQueue::prune()
{
    {
        QMutexLocker locker(&m_mutex);
        m_items.removeIf([](const Item &it) {
            return it.status == StatusCommitted;
        });
    }
    emit queueChanged();
}

void CommitQueue::removeCommitted(const QList<Item> &items)
{
    QSet<QString> toRemove;
    for (const Item &it : items) {
        toRemove.insert(it.path + QString::number(it.queuedAt));
    }
    m_items.removeIf([&](const Item &mi) {
        return toRemove.contains(mi.path + QString::number(mi.queuedAt));
    });
}

QString CommitQueue::toJson() const
{
    QMutexLocker locker(&m_mutex);
    QJsonArray arr;
    for (const Item &it : m_items) {
        QJsonObject obj;
        obj["path"] = it.path;
        obj["fromPath"] = it.fromPath;
        obj["operation"] = it.operation;
        obj["operationName"] = operationName(it.operation);
        obj["status"] = it.status;
        obj["statusName"] = statusName(it.status);
        obj["queuedAt"] = it.queuedAt;
        obj["retryCount"] = it.retryCount;
        arr.append(obj);
    }
    return QJsonDocument(arr).toJson(QJsonDocument::Indented);
}

QString CommitQueue::operationName(int op)
{
    switch (op) {
        case OpModify: return "Modify";
        case OpAdd:    return "Add";
        case OpDelete: return "Delete";
        case OpMove:   return "Move";
    }
    return "Unknown";
}

QString CommitQueue::statusName(int st)
{
    switch (st) {
        case StatusPending:    return "Pending";
        case StatusInProgress: return "InProgress";
        case StatusCommitted:  return "Committed";
        case StatusFailed:     return "Failed";
    }
    return "Unknown";
}
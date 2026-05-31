#include "commitqueue.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QStandardPaths>

CommitQueue &CommitQueue::instance()
{
    static CommitQueue inst;
    return inst;
}

CommitQueue::CommitQueue(QObject *parent)
    : QObject(parent)
{
    // DataPath: ~/.local/share/SVNFileBox/commit_queue/
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty())
        dataDir = QDir::home().filePath(".local/share/SVNFileBox");
    m_queuePath = dataDir + "/commit_queue";
    QDir().mkpath(m_queuePath);

    load();
}

QString CommitQueue::queueFilePath() const
{
    return m_queuePath + "/pending_queue.json";
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

    save();
    emit queueChanged();
}

void CommitQueue::enqueueMove(const QString &fromPath, const QString &toPath)
{
    enqueue(toPath, OpMove, fromPath);
}

QList<CommitQueue::Item> CommitQueue::resolve()
{
    QMutexLocker locker(&m_mutex);

    // Last-wins map
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

    // Execution order: Delete → Move → Add → Modify
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
        QSet<QString> toRemove;
        for (const Item &it : items) {
            toRemove.insert(it.path + QString::number(it.queuedAt));
        }
        m_items.removeIf([&](const Item &mi) {
            return toRemove.contains(mi.path + QString::number(mi.queuedAt));
        });
    }
    save();
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
    save();
    emit queueChanged();
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
    save();
    emit queueChanged();
}

void CommitQueue::save()
{
    QMutexLocker locker(&m_mutex);
    saveInternal();
}

void CommitQueue::saveInternal()
{
    QString fp = queueFilePath();
    QJsonArray arr;
    for (const Item &it : m_items) {
        QJsonObject obj;
        obj["path"] = it.path;
        obj["fromPath"] = it.fromPath;
        obj["operation"] = it.operation;
        obj["status"] = it.status;
        obj["queuedAt"] = it.queuedAt;
        obj["retryCount"] = it.retryCount;
        arr.append(obj);
    }
    QJsonDocument doc(arr);
    QFile f(fp);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(doc.toJson(QJsonDocument::Indented));
    }
}

void CommitQueue::load()
{
    QString fp = queueFilePath();
    QFile f(fp);
    if (!f.exists()) return;
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return;

    QMutexLocker locker(&m_mutex);
    m_items.clear();
    for (const QJsonValue &val : doc.array()) {
        QJsonObject obj = val.toObject();
        Item it;
        it.path = obj["path"].toString();
        it.fromPath = obj["fromPath"].toString();
        it.operation = obj["operation"].toInt();
        it.status = obj["status"].toInt();
        it.queuedAt = obj["queuedAt"].toVariant().toLongLong();
        it.retryCount = obj["retryCount"].toInt();
        m_items.append(it);
    }
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

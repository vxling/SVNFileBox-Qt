#include "syncrecordservice.h"
#include "sqlitesyncrecordstore.h"
#include <QCoreApplication>
#include <QDebug>

SyncRecordService *SyncRecordService::instance()
{
    static SyncRecordService inst;
    return &inst;
}

SyncRecordService::SyncRecordService(QObject *parent)
    : QAbstractListModel(parent)
    , m_store(SqliteSyncRecordStore::instance())
{
    // Connect to store's recordsChanged to refresh our in-memory list
    connect(m_store, &SqliteSyncRecordStore::recordsChanged,
            this, &SyncRecordService::refreshAll, Qt::QueuedConnection);
}

void SyncRecordService::refreshAll()
{
    // Reload all records from SQLite into memory
    beginResetModel();
    qDeleteAll(m_records);
    m_records.clear();
    QList<SyncRecord*> recs = m_store->getAllRecords(1000);
    for (SyncRecord *r : recs) {
        m_records.append(r);
    }
    endResetModel();
}

void SyncRecordService::loadRecordsForRepo(const QString &repoName)
{
    beginResetModel();
    qDeleteAll(m_records);
    m_records.clear();
    QList<SyncRecord*> recs = m_store->getRecords(repoName, 1000);
    for (SyncRecord *r : recs) {
        m_records.append(r);
    }
    endResetModel();
}

int SyncRecordService::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_records.size();
}

QVariant SyncRecordService::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_records.size())
        return QVariant();

    SyncRecord *r = m_records.at(index.row());
    switch (role) {
        case IdRole:          return QVariant::fromValue(r->id());
        case RepoNameRole:    return r->repoName();
        case FilePathRole:    return r->filePath();
        case OperationRole:   return r->operation();
        case ResultRole:      return r->result();
        case MessageRole:     return r->message();
        case TimestampRole:   return r->timestamp();
    }
    return QVariant();
}

QHash<int, QByteArray> SyncRecordService::roleNames() const
{
    return {
        { IdRole,        "id" },
        { RepoNameRole,  "repoName" },
        { FilePathRole,  "filePath" },
        { OperationRole, "operation" },
        { ResultRole,    "result" },
        { MessageRole,   "message" },
        { TimestampRole, "timestamp" }
    };
}

void SyncRecordService::addRecord(const QString &repo, const QString &file,
                                  const QString &op, const QString &result,
                                  const QString &message)
{
    // Insert into SQLite
    m_store->addRecord(repo, QDateTime::currentMSecsSinceEpoch(),
                       file, op, result, message);

    // Also insert into in-memory list at front (capped at 1000)
    beginInsertRows(QModelIndex(), 0, 0);
    SyncRecord *r = new SyncRecord(repo, file, op, result, message, this);
    r->setTimestamp(QDateTime::currentDateTime());
    m_records.prepend(r);
    if (m_records.size() > 1000) {
        delete m_records.takeLast();
    }
    endInsertRows();
    emit recordAdded();
}

void SyncRecordService::clear()
{
    beginResetModel();
    qDeleteAll(m_records);
    m_records.clear();
    endResetModel();
}

QVariantList SyncRecordService::recordsAsList() const
{
    QVariantList list;
    for (SyncRecord *r : m_records) {
        list.append(QVariant::fromValue(r));
    }
    return list;
}

void SyncRecordService::clearRecords()
{
    clear();
}

SyncRecord *SyncRecordService::getRecord(int index) const
{
    if (index < 0 || index >= m_records.size()) return nullptr;
    return m_records.at(index);
}

QVariantList SyncRecordService::getRecordsForRepo(const QString &repoName, int limit) const
{
    QVariantList list;
    QList<SyncRecord*> recs = m_store->getRecords(repoName, limit);
    for (SyncRecord *r : recs) {
        list.append(QVariant::fromValue(r));
    }
    return list;
}

void SyncRecordService::deleteRepoRecords(const QString &repoName)
{
    m_store->deleteRepo(repoName);
}
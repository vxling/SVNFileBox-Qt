#include "syncrecordservice.h"
#include <QCoreApplication>
#include <QDebug>

SyncRecordService *SyncRecordService::instance()
{
    static SyncRecordService inst;
    return &inst;
}

SyncRecordService::SyncRecordService(QObject *parent)
    : QAbstractListModel(parent)
{
    load();
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
        { RepoNameRole, "repoName" },
        { FilePathRole, "filePath" },
        { OperationRole, "operation" },
        { ResultRole, "result" },
        { MessageRole, "message" },
        { TimestampRole, "timestamp" }
    };
}

void SyncRecordService::addRecord(const QString &repo, const QString &file,
                                  const QString &op, const QString &result,
                                  const QString &message)
{
    beginInsertRows(QModelIndex(), 0, 0);
    SyncRecord *r = new SyncRecord(repo, file, op, result, message, this);
    m_records.prepend(r);
    if (m_records.size() > 500) {
        delete m_records.takeLast();
    }
    endInsertRows();
    save();
    emit recordAdded();
}

void SyncRecordService::clear()
{
    beginResetModel();
    qDeleteAll(m_records);
    m_records.clear();
    endResetModel();
    save();
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

QString SyncRecordService::configDir() const
{
    return QDir::home().filePath(".svnfilebox");
}

void SyncRecordService::load()
{
    QFile f(configDir() + "/sync_records.json");
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    QJsonArray arr = doc.array();
    for (const QJsonValue &v : arr) {
        QJsonObject o = v.toObject();
        SyncRecord *r = new SyncRecord(
            o["repo"].toString(),
            o["file"].toString(),
            o["op"].toString(),
            o["result"].toString(),
            o["msg"].toString(),
            const_cast<SyncRecordService*>(this));
        m_records.append(r);
    }
}

void SyncRecordService::save()
{
    QDir d(configDir());
    if (!d.exists()) d.mkpath(configDir());
    QJsonArray arr;
    for (SyncRecord *r : m_records) {
        arr.append(QJsonObject{
            {"repo", r->repoName()},
            {"file", r->filePath()},
            {"op", r->operation()},
            {"result", r->result()},
            {"msg", r->message()}
        });
    }
    QFile f(configDir() + "/sync_records.json");
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(arr).toJson());
        f.close();
    }
}

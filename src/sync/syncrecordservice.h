#ifndef SYNCRECORDSERVICE_H
#define SYNCRECORDSERVICE_H

#include <QObject>
#include <QAbstractListModel>
#include <QList>
#include <QVariantList>
#include "syncrecord.h"

class SqliteSyncRecordStore;

class SyncRecordService : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        RepoNameRole,
        FilePathRole,
        OperationRole,
        ResultRole,
        MessageRole,
        TimestampRole
    };

    static SyncRecordService *instance();
    explicit SyncRecordService(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addRecord(const QString &repo, const QString &file,
                               const QString &op, const QString &result,
                               const QString &message = QString());
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariantList recordsAsList() const;
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE SyncRecord *getRecord(int index) const;
    Q_INVOKABLE int recordCount() const { return m_records.size(); }

    /// Load records for a specific repo into the in-memory collection
    Q_INVOKABLE void loadRecordsForRepo(const QString &repoName);
    /// Get records for a specific repo from SQLite
    Q_INVOKABLE QVariantList getRecordsForRepo(const QString &repoName, int limit = 1000) const;
    /// Delete all records for a repository
    Q_INVOKABLE void deleteRepoRecords(const QString &repoName);

signals:
    void recordAdded();

private:
    void loadFromSqlite(const QString &repoName);
    void refreshAll();

    SqliteSyncRecordStore *m_store;
    QList<SyncRecord *> m_records;
};

#endif // SYNCRECORDSERVICE_H
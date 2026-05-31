#ifndef SQLITESYNCRECORDSTORE_H
#define SQLITESYNCRECORDSTORE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QSqlDatabase>
#include "syncrecord.h"

/// SQLite-backed sync record store — single shared table.
/// Schema: sync_records(id INTEGER PRIMARY KEY AUTOINCREMENT, repo_name TEXT NOT NULL,
///                     timestamp TEXT NOT NULL, file_path TEXT NOT NULL,
///                     operation TEXT NOT NULL, result TEXT NOT NULL, message TEXT NOT NULL DEFAULT '')
/// Retention: MaxAgeDays = 10, MaxRecordsPerRepo = 10,000.
class SqliteSyncRecordStore : public QObject
{
    Q_OBJECT

public:
    static SqliteSyncRecordStore *instance();

    /// Adds a record. Auto-trims oldest if over MaxRecordsPerRepo per repo.
    Q_INVOKABLE void addRecord(const QString &repoName, qlonglong timestampMs,
                               const QString &filePath, const QString &operation,
                               const QString &result, const QString &message = QString());

    /// Returns records for a repo, newest first.
    Q_INVOKABLE QList<SyncRecord*> getRecords(const QString &repoName, int limit = 1000);

    /// Returns all records across all repos, newest first.
    Q_INVOKABLE QList<SyncRecord*> getAllRecords(int limit = 1000);

    /// Deletes all records for a repository.
    Q_INVOKABLE void deleteRepo(const QString &repoName);

    /// Runs cleanup: removes records older than MaxAgeDays and trims each repo to MaxRecordsPerRepo.
    Q_INVOKABLE void cleanupAll();

signals:
    void recordsChanged();

private:
    explicit SqliteSyncRecordStore(QObject *parent = nullptr);
    ~SqliteSyncRecordStore() override;
    void trimByCount(const QString &repoName);

    static const int MaxAgeDays = 10;
    static const int MaxRecordsPerRepo = 10000;

    QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // SQLITESYNCRECORDSTORE_H
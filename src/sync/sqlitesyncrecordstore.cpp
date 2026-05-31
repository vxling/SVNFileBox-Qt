#include "sqlitesyncrecordstore.h"
#include <QCoreApplication>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>

SqliteSyncRecordStore *SqliteSyncRecordStore::instance()
{
    static SqliteSyncRecordStore inst;
    return &inst;
}

SqliteSyncRecordStore::SqliteSyncRecordStore(QObject *parent)
    : QObject(parent)
    , m_dbPath(QDir::home().absoluteFilePath(".svnfilebox/sync_records.db"))
{
    // Use a unique connection name per process
    m_db = QSqlDatabase::addDatabase("QSQLITE", "sync_records");
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qWarning() << "[SqliteSyncRecordStore] Failed to open DB:" << m_db.lastError();
        return;
    }

    // Create table with indexes
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS sync_records ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "repo_name TEXT NOT NULL,"
           "timestamp TEXT NOT NULL,"
           "file_path TEXT NOT NULL,"
           "operation TEXT NOT NULL,"
           "result TEXT NOT NULL,"
           "message TEXT NOT NULL DEFAULT '')");
    q.exec("CREATE INDEX IF NOT EXISTS idx_repo ON sync_records(repo_name)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_ts ON sync_records(timestamp)");

    qDebug() << "[SqliteSyncRecordStore] Opened" << m_dbPath;

    // Run cleanup on startup
    cleanupAll();
}

SqliteSyncRecordStore::~SqliteSyncRecordStore() = default;

void SqliteSyncRecordStore::addRecord(const QString &repoName, qlonglong timestampMs,
                                      const QString &filePath, const QString &operation,
                                      const QString &result, const QString &message)
{
    if (!m_db.isOpen()) return;

    QSqlQuery q(m_db);
    q.prepare("INSERT INTO sync_records (repo_name, timestamp, file_path, operation, result, message) "
              "VALUES (:repo, :ts, :path, :op, :result, :msg)");
    q.bindValue(":repo", repoName);
    q.bindValue(":ts", QDateTime::fromMSecsSinceEpoch(timestampMs).toString(Qt::ISODateWithMs));
    q.bindValue(":path", filePath);
    q.bindValue(":op", operation);
    q.bindValue(":result", result);
    q.bindValue(":msg", message);
    if (!q.exec()) {
        qWarning() << "[SqliteSyncRecordStore] Insert failed:" << q.lastError();
        return;
    }

    trimByCount(repoName);
    emit recordsChanged();
}

QList<SyncRecord*> SqliteSyncRecordStore::getRecords(const QString &repoName, int limit)
{
    QList<SyncRecord*> records;
    if (!m_db.isOpen()) return records;

    QSqlQuery q(m_db);
    q.prepare("SELECT id, repo_name, timestamp, file_path, operation, result, message "
              "FROM sync_records WHERE repo_name = :repo ORDER BY id DESC LIMIT :limit");
    q.bindValue(":repo", repoName);
    q.bindValue(":limit", limit);
    if (!q.exec()) return records;

    while (q.next()) {
        SyncRecord *r = new SyncRecord(q.value(1).toString(), q.value(3).toString(),
                                       q.value(4).toString(), q.value(5).toString(),
                                       q.value(6).toString(), nullptr);
        r->setTimestamp(QDateTime::fromString(q.value(2).toString(), Qt::ISODateWithMs));
        records.append(r);
    }
    return records;
}

QList<SyncRecord*> SqliteSyncRecordStore::getAllRecords(int limit)
{
    QList<SyncRecord*> records;
    if (!m_db.isOpen()) return records;

    QSqlQuery q(m_db);
    q.prepare("SELECT id, repo_name, timestamp, file_path, operation, result, message "
              "FROM sync_records ORDER BY id DESC LIMIT :limit");
    q.bindValue(":limit", limit);
    if (!q.exec()) return records;

    while (q.next()) {
        SyncRecord *r = new SyncRecord(q.value(1).toString(), q.value(3).toString(),
                                       q.value(4).toString(), q.value(5).toString(),
                                       q.value(6).toString(), nullptr);
        r->setTimestamp(QDateTime::fromString(q.value(2).toString(), Qt::ISODateWithMs));
        records.append(r);
    }
    return records;
}

void SqliteSyncRecordStore::deleteRepo(const QString &repoName)
{
    if (!m_db.isOpen()) return;
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM sync_records WHERE repo_name = :repo");
    q.bindValue(":repo", repoName);
    q.exec();
    qDebug() << "[SqliteSyncRecordStore] Deleted records for repo:" << repoName;
    emit recordsChanged();
}

void SqliteSyncRecordStore::cleanupAll()
{
    if (!m_db.isOpen()) return;

    // Trim by age (10 days)
    qlonglong cutoff = QDateTime::currentDateTime().addDays(-MaxAgeDays).toMSecsSinceEpoch();
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM sync_records WHERE timestamp < :cutoff");
    q.bindValue(":cutoff", QDateTime::fromMSecsSinceEpoch(cutoff).toString(Qt::ISODateWithMs));
    q.exec();
    qDebug() << "[SqliteSyncRecordStore] Cleanup done";

    // Trim by count per repo
    QSqlQuery repoQ(m_db);
    repoQ.exec("SELECT DISTINCT repo_name FROM sync_records");
    while (repoQ.next()) {
        trimByCount(repoQ.value(0).toString());
    }
}

void SqliteSyncRecordStore::trimByCount(const QString &repoName)
{
    if (!m_db.isOpen()) return;
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM sync_records WHERE repo_name = :repo AND id NOT IN "
              "(SELECT id FROM sync_records WHERE repo_name = :repo2 ORDER BY id DESC LIMIT :limit)");
    q.bindValue(":repo", repoName);
    q.bindValue(":repo2", repoName);
    q.bindValue(":limit", MaxRecordsPerRepo);
    q.exec();
}
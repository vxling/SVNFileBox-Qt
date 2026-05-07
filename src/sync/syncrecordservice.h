#ifndef SYNCRECORDSERVICE_H
#define SYNCRECORDSERVICE_H

#include <QObject>
#include <QAbstractListModel>
#include <QList>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include "syncrecord.h"

class SyncRecordService : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        RepoNameRole = Qt::UserRole + 1,
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

signals:
    void recordAdded();

private:
    void load();
    void save();
    QString configDir() const;

    QList<SyncRecord *> m_records;
};

#endif // SYNCRECORDSERVICE_H

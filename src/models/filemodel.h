#ifndef FILEMODEL_H
#define FILEMODEL_H

#include <QAbstractListModel>
#include <QVariantList>

class FileItem
{
public:
    QString name;
    QString path;
    QString status; // "synced", "modified", "new", "deleted"
    QString modifiedTime;
};

class FileModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles { NameRole = Qt::UserRole + 1, PathRole, StatusRole, ModifiedRole };

    explicit FileModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addFile(const QString &name, const QString &path, const QString &status);
    Q_INVOKABLE void updateStatus(int row, const QString &status);
    Q_INVOKABLE void clear();

private:
    QList<FileItem> m_files;
};

#endif // FILEMODEL_H

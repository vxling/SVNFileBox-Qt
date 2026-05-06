#include "filemodel.h"

FileModel::FileModel(QObject *parent) : QAbstractListModel(parent) {}

int FileModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_files.size();
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_files.size())
        return QVariant();

    const FileItem &item = m_files.at(index.row());
    switch (role) {
        case NameRole: return item.name;
        case PathRole: return item.path;
        case StatusRole: return item.status;
        case ModifiedRole: return item.modifiedTime;
    }
    return QVariant();
}

QHash<int, QByteArray> FileModel::roleNames() const
{
    return { {NameRole, "name"}, {PathRole, "path"}, {StatusRole, "status"}, {ModifiedRole, "modifiedTime"} };
}

void FileModel::addFile(const QString &name, const QString &path, const QString &status)
{
    beginInsertRows(QModelIndex(), m_files.size(), m_files.size());
    m_files.append({name, path, status, ""});
    endInsertRows();
}

void FileModel::updateStatus(int row, const QString &status)
{
    if (row >= 0 && row < m_files.size()) {
        m_files[row].status = status;
        emit dataChanged(index(row), index(row), {StatusRole});
    }
}

void FileModel::clear()
{
    beginResetModel();
    m_files.clear();
    endResetModel();
}

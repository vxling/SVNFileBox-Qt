#include "repolistmodel.h"
#include <QDebug>

RepoListModel::RepoListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RepoListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_repos.size();
}

QVariant RepoListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_repos.size())
        return QVariant();

    QVariantMap repo = m_repos[index.row()].toMap();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::UserRole + 1: // name
        return repo["name"].toString();
    case Qt::UserRole + 2: // path
        return repo["path"].toString();
    case Qt::UserRole + 3: // url
        return repo["url"].toString();
    case Qt::UserRole + 4: // type
        return repo["type"].toString();
    case Qt::UserRole + 5: // isSelected
        return repo["isSelected"].toBool();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> RepoListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::UserRole + 1] = "name";
    roles[Qt::UserRole + 2] = "path";
    roles[Qt::UserRole + 3] = "url";
    roles[Qt::UserRole + 4] = "type";
    roles[Qt::UserRole + 5] = "isSelected";
    return roles;
}

void RepoListModel::loadFromConfig(const QVariantList &repos)
{
    beginResetModel();
    m_repos = repos;
    endResetModel();
}

void RepoListModel::selectRepo(int index)
{
    if (index < 0 || index >= m_repos.size()) return;
    m_selectedIndex = index;
    QVariantMap repo = m_repos[index].toMap();
    emit repoSelected(index, repo["path"].toString(), repo["url"].toString());
}

void RepoListModel::removeRepo(int index)
{
    if (index < 0 || index >= m_repos.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_repos.removeAt(index);
    endRemoveRows();
}

QString RepoListModel::repoName(int index) const
{
    if (index < 0 || index >= m_repos.size()) return QString();
    return m_repos[index].toMap()["name"].toString();
}

QString RepoListModel::repoPath(int index) const
{
    if (index < 0 || index >= m_repos.size()) return QString();
    return m_repos[index].toMap()["path"].toString();
}
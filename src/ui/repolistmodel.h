#pragma once

#include <QAbstractListModel>
#include <QVariantList>

class RepoListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit RepoListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadFromConfig(const QVariantList &repos);
    Q_INVOKABLE void selectRepo(int index);
    Q_INVOKABLE void removeRepo(int index);
    Q_INVOKABLE int count() const { return m_repos.size(); }
    Q_INVOKABLE QString repoName(int index) const;
    Q_INVOKABLE QString repoPath(int index) const;

signals:
    void repoSelected(int index, const QString &path, const QString &url);

private:
    QVariantList m_repos;
    int m_selectedIndex = -1;
};
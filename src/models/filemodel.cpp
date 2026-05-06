#include "filemodel.h"
#include <QDir>
#include <QUrl>

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
        case NameRole:         return item.name;
        case FullPathRole:     return item.fullPath;
        case IsDirectoryRole:  return item.isDirectory;
        case SvnStatusRole:    return item.svnStatus;
        case FileSizeDisplayRole: return formatFileSize(item.fileSize);
        case LastModifiedDisplayRole:
            return item.lastModified.isValid()
                ? item.lastModified.toString("yyyy-MM-dd HH:mm")
                : QString();
        case IsCurrentPathRole: return item.isCurrentPath;
    }
    return QVariant();
}

QHash<int, QByteArray> FileModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { FullPathRole, "fullPath" },
        { IsDirectoryRole, "isDirectory" },
        { SvnStatusRole, "svnStatus" },
        { FileSizeDisplayRole, "fileSizeDisplay" },
        { LastModifiedDisplayRole, "lastModifiedDisplay" },
        { IsCurrentPathRole, "isCurrentPath" }
    };
}

void FileModel::load(const QString &path)
{
    beginResetModel();
    m_files.clear();
    m_currentPath = path;

    QDir dir(path);
    if (!dir.exists()) {
        endResetModel();
        return;
    }

    // 添加 "返回上级目录" 行
    if (path != QDir::rootPath()) {
        FileItem up;
        up.name = "..";
        up.fullPath = QDir(path).absoluteFilePath("..");
        up.isDirectory = true;
        up.isCurrentPath = true;
        up.svnStatus = "Hidden";
        m_files.append(up);
    }

    // 列出文件
    QFileInfoList infos = dir.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
        QDir::DirsFirst | QDir::Name
    );

    for (const QFileInfo &fi : infos) {
        if (fi.fileName() == ".svn")
            continue;
        FileItem item;
        item.name = fi.fileName();
        item.fullPath = fi.absoluteFilePath();
        item.isDirectory = fi.isDir();
        item.fileSize = fi.size();
        item.lastModified = fi.lastModified();
        item.isCurrentPath = false;
        item.svnStatus = "Normal";  // TODO: 从 SVNClient 获取真实状态
        m_files.append(item);
    }

    endResetModel();
}

QString FileModel::getFilePath(int row) const
{
    if (row >= 0 && row < m_files.size())
        return m_files.at(row).fullPath;
    return QString();
}

void FileModel::clear()
{
    beginResetModel();
    m_files.clear();
    endResetModel();
}

QString FileModel::formatFileSize(qint64 bytes)
{
    if (bytes < 0) return QString();
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024 * 1024), 'f', 1) + " GB";
}

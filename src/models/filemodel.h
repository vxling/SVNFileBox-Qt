#ifndef FILEMODEL_H
#define FILEMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QFileInfo>
#include <QDateTime>
#include <QString>

class SVNClient;
class FileCopier;

class FileItem
{
public:
    QString name;
    QString fullPath;
    bool isDirectory;
    qint64 fileSize;
    QDateTime lastModified;
    QString svnStatus;      // Normal/Modified/Added/Deleted/Conflicted/Unversioned/Missing/Hidden
    bool isCurrentPath;     // "返回上级目录" 行
};

class FileModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentPath READ currentPath WRITE setCurrentPath NOTIFY currentPathChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        FullPathRole,
        IsDirectoryRole,
        SvnStatusRole,
        FileSizeDisplayRole,
        LastModifiedDisplayRole,
        IsCurrentPathRole,
        TypeDisplayRole
    };

    explicit FileModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void load(const QString &path);
    Q_INVOKABLE QString getFilePath(int row) const;
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool createDirectory(const QString &path);
    Q_INVOKABLE bool createFile(const QString &fullPath);
    Q_INVOKABLE QString pasteFromClipboard();
    Q_INVOKABLE QString importFiles(const QStringList &paths, const QString &destPath);

    // New: plan + async copy with progress. Returns immediately; emits
    // copyProgress + copyCompleted. UI displays progress via signals.
    Q_INVOKABLE void importFilesAsync(const QStringList &paths, const QString &destPath);
    Q_INVOKABLE void cancelCopy();
    Q_INVOKABLE QString formatBytes(qint64 bytes) const;

    void setSvnClient(SVNClient *client) { m_svnClient = client; }
    SVNClient *svnClient() const { return m_svnClient; }

    QString currentPath() const { return m_currentPath; }
    void setCurrentPath(const QString &path);

signals:
    void currentPathChanged();
    // Copy progress (UI can show progress bar / file count)
    void copyProgress(int currentIndex, int totalCount, qint64 bytesCopied, qint64 totalBytes, const QString &currentFile);
    void copyCompleted(int copiedCount, int skippedCount, int overwrittenCount, const QString &errorMessage);

private:
    QList<FileItem> m_files;
    QString m_currentPath;
    SVNClient *m_svnClient = nullptr;
    FileCopier *m_copier = nullptr;

    static QString formatFileSize(qint64 bytes);
    static QString getTypeDisplay(const QString &fileName, bool isDir, bool isCurrentPath);
    bool copyDirectory(const QString &src, const QString &dst);
    void collectNewFiles(const QString &dirPath, QStringList &out);
};

#endif // FILEMODEL_H

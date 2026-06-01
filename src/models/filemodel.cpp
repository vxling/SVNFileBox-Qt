#include "filemodel.h"
#include "svn/svnclient.h"
#include "sync/commitqueue.h"
#include "services/newfileservice.h"
#include "services/fileanalyzer.h"
#include "services/filecopier.h"
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QClipboard>
#include <QGuiApplication>
#include <QFile>
#include <QMimeData>
#include <QDebug>

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
        case TypeDisplayRole:  return getTypeDisplay(item.name, item.isDirectory, item.isCurrentPath);
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
        { IsCurrentPathRole, "isCurrentPath" },
        { TypeDisplayRole, "typeDisplay" }
    };
}

void FileModel::setCurrentPath(const QString &path) {
    if (m_currentPath != path) {
        m_currentPath = path;
        emit currentPathChanged();
    }
}

void FileModel::load(const QString &path)
{
    beginResetModel();
    m_files.clear();
    m_currentPath = path;
    emit currentPathChanged();

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
        item.svnStatus = m_svnClient
            ? m_svnClient->getStatusString(fi.absoluteFilePath())
            : QString("Normal");
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

bool FileModel::createDirectory(const QString &path)
{
    QDir dir;
    if (!dir.mkpath(path))
        return false;
    m_svnClient->add(path);
    bool ok = m_svnClient->commit(path, "[SVNFileBox] Add folder: " + QFileInfo(path).fileName());
    CommitQueue::instance().enqueue(path, CommitQueue::OpAdd);
    return ok;
}

bool FileModel::createFile(const QString &fullPath)
{
    if (!NewFileService::create(fullPath))
        return false;
    m_svnClient->add(fullPath);
    bool ok = m_svnClient->commit(fullPath, "[SVNFileBox] Add file: " + QFileInfo(fullPath).fileName());
    CommitQueue::instance().enqueue(fullPath, CommitQueue::OpAdd);
    return ok;
}

QString FileModel::pasteFromClipboard()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard) return QString();

    const QMimeData *mime = clipboard->mimeData();
    if (!mime) return QString();

    // Try to get file URLs from clipboard
    QStringList files;
    for (const QUrl &url : mime->urls()) {
        if (url.isLocalFile()) {
            files.append(url.toLocalFile());
        }
    }

    if (files.isEmpty()) return QString();

    // Copy first file from clipboard to current directory
    QString src = files.first();
    QString name = src.split("/").last();
    QString dst = m_currentPath + "/" + name;

    if (QFile::copy(src, dst)) {
        // svn add and enqueue for debounced commit
        m_svnClient->add(dst);
        CommitQueue::instance().enqueue(dst, CommitQueue::OpAdd);
        return dst;
    }
    return QString();
}

QString FileModel::importFiles(const QStringList &paths, const QString &destPath)
{
    if (paths.isEmpty() || destPath.isEmpty()) return QString();

    // Collect all new items for svn add
    QStringList newItems;

    for (const QString &srcPath : paths) {
        QFileInfo info(srcPath);
        QString name = info.fileName();
        QString dstPath = destPath + "/" + name;

        if (info.isDir()) {
            // Recursive copy directory
            if (!copyDirectory(srcPath, dstPath)) continue;
        } else {
            if (!QFile::copy(srcPath, dstPath)) continue;
        }

        newItems.append(dstPath);
        // If directory, also add all children
        if (info.isDir()) {
            collectNewFiles(dstPath, newItems);
        }
    }

    if (newItems.isEmpty()) return QString();

    // svn add all new items, then enqueue for commit (not immediate commit)
    for (const QString &item : newItems) {
        m_svnClient->add(item);
        // Enqueue each new file for debounced commit via CommitQueue
        CommitQueue::instance().enqueue(item, CommitQueue::OpAdd);
    }

    return newItems.join(", ");
}

bool FileModel::copyDirectory(const QString &src, const QString &dst)
{
    QDir srcDir(src);
    if (!srcDir.mkpath(dst)) return false;

    QDir dstDir(dst);
    for (const QFileInfo &entry : srcDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        if (entry.isDir()) {
            if (!copyDirectory(entry.filePath(), dst + "/" + entry.fileName())) return false;
        } else {
            if (!QFile::copy(entry.filePath(), dst + "/" + entry.fileName())) return false;
        }
    }
    return true;
}

void FileModel::collectNewFiles(const QString &dirPath, QStringList &out)
{
    QDir dir(dirPath);
    for (const QFileInfo &entry : dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        QString full = entry.filePath();
        out.append(full);
        if (entry.isDir()) {
            collectNewFiles(full, out);
        }
    }
}

void FileModel::importFilesAsync(const QStringList &paths, const QString &destPath)
{
    if (paths.isEmpty() || destPath.isEmpty()) return;

    // Lazy-init copier
    if (!m_copier) {
        m_copier = new FileCopier(this);
        connect(m_copier, &FileCopier::copyProgress, this,
                [this](const CopyProgress &p) {
            emit copyProgress(p.currentIndex, p.totalCount, p.bytesCopied,
                              p.totalBytes, p.currentFile);
        });
        connect(m_copier, &FileCopier::copyCompleted, this,
                [this, destPath](const CopyResult &r) {
            if (!r.wasCancelled && r.copiedCount > 0 && m_svnClient) {
                // svn add all newly-copied paths + enqueue commit
                for (const QString &p : r.svnAddedPaths) {
                    m_svnClient->add(p);
                    CommitQueue::instance().enqueue(p, CommitQueue::OpAdd);
                }
            }
            emit copyCompleted(r.copiedCount, r.skippedCount, r.overwrittenCount,
                               r.errorMessage);
        });
    }

    // Phase 1: analyze → plan
    FileCopyPlan plan = FileAnalyzer::analyze(paths, destPath);
    if (plan.isSameLocation) {
        CopyResult r;
        r.errorMessage = "源和目标位置相同";
        emit copyCompleted(0, 0, 0, r.errorMessage);
        return;
    }
    if (plan.items.isEmpty()) {
        emit copyCompleted(0, 0, 0, QString());
        return;
    }

    // Phase 2: async copy. svnAddPaths = all dest paths (incl. dirs)
    QStringList allDest;
    for (const FileCopyItem &it : plan.items) {
        allDest.append(it.destPath);
    }
    m_copier->copyPlanAsync(plan, allDest);
}

void FileModel::cancelCopy()
{
    if (m_copier) m_copier->cancel();
}

QString FileModel::formatBytes(qint64 bytes) const
{
    return FileCopyPlan::formatBytes(bytes);
}

QString FileModel::formatFileSize(qint64 bytes)
{
    if (bytes < 0) return QString();
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024 * 1024), 'f', 1) + " GB";
}

QString FileModel::getTypeDisplay(const QString &fileName, bool isDir, bool isCurrentPath)
{
    if (isCurrentPath) return QString();
    if (isDir) return "文件夹";

    auto ext = fileName.contains('.')
        ? fileName.split('.').last().toLower()
        : QString();

    if (ext.isEmpty()) return "文档";

    // Code
    if (ext == "cs" || ext == "fs" || ext == "vb" || ext == "java" || ext == "py"
        || ext == "go" || ext == "rs" || ext == "c" || ext == "cpp" || ext == "h" || ext == "hpp"
        || ext == "swift" || ext == "kt" || ext == "rb" || ext == "php" || ext == "js" || ext == "ts")
        return "代码";

    // Excel
    if (ext == "xlsx" || ext == "xls" || ext == "xlsm" || ext == "csv")
        return "Excel";

    // Word
    if (ext == "docx" || ext == "doc" || ext == "odt" || ext == "rtf")
        return "Word";

    // PPT
    if (ext == "pptx" || ext == "ppt" || ext == "odp")
        return "PPT";

    // PDF
    if (ext == "pdf")
        return "PDF";

    // Image
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp"
        || ext == "webp" || ext == "ico" || ext == "svg" || ext == "tiff" || ext == "tif")
        return "图片";

    // Video
    if (ext == "mp4" || ext == "avi" || ext == "mkv" || ext == "mov" || ext == "wmv" || ext == "flv" || ext == "webm")
        return "视频";

    // Audio
    if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "aac" || ext == "ogg" || ext == "wma")
        return "音频";

    // Archive
    if (ext == "zip" || ext == "rar" || ext == "7z" || ext == "tar" || ext == "gz" || ext == "bz2")
        return "压缩包";

    // Text
    if (ext == "txt" || ext == "md" || ext == "log" || ext == "ini" || ext == "cfg" || ext == "conf")
        return "文本";

    // Config
    if (ext == "json" || ext == "xml" || ext == "yaml" || ext == "yml" || ext == "toml")
        return "配置";

    // Web
    if (ext == "html" || ext == "htm" || ext == "css" || ext == "js" || ext == "ts"
        || ext == "jsx" || ext == "tsx" || ext == "vue" || ext == "sass" || ext == "scss")
        return "Web";

    // Executable
    if (ext == "exe" || ext == "msi" || ext == "dll" || ext == "sys" || ext == "bat" || ext == "cmd" || ext == "ps1"
        || ext == "app" || ext == "dmg" || ext == "deb" || ext == "rpm" || ext == "appimage")
        return "可执行";

    return "文档";
}

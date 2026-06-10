#pragma once

#include <QMainWindow>
#include <QTableView>
#include <QListView>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QStandardItemModel>
#include <QLineEdit>

class ConfigService;
class FileModel;
class SyncEngine;
class RepoListModel;

namespace SVNFileBox {
class RepoGlobalManager;
}

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ConfigService *configService,
                        FileModel *fileModel,
                        SyncEngine *syncEngine,
                        SVNFileBox::RepoGlobalManager *globalManager,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onRepoSelected(const QModelIndex &index);
    void onFileDoubleClicked(const QModelIndex &index);
    void onFileContextMenu(const QPoint &pos);
    void compressToZip(const QString &sourcePath, const QString &zipPath, bool isDir);
    void createNewFile(const QString &dir, const QString &ext);
    void createNewOfficeFile(const QString &dir);
    void createNewImage(const QString &dir, const QString &format);
    void onRefreshClicked();
    void onGoUpClicked();
    void onPathEditingFinished();
    void onManualSync();
    void onFilesChanged();
    void onRepositoryFocused(const QString &path);
    void onSyncNotification(const QString &message);
    void onConflictDetected(const QStringList &files);
    void onSyncStarted();
    void onCopyProgress(int currentIndex, int totalCount, qint64 bytesCopied, qint64 totalBytes, const QString &currentFile);
    void onCopyCompleted(int copiedCount, int skippedCount, int overwrittenCount, const QString &errorMessage);

private:
    void setupUiFromCode();
    void connectSignals();
    void navigateTo(const QString &path);
    void goUp();

    ConfigService *m_configService = nullptr;
    FileModel *m_fileModel = nullptr;
    SyncEngine *m_syncEngine = nullptr;
    SVNFileBox::RepoGlobalManager *m_globalManager = nullptr;

    Ui::MainWindow *ui = nullptr;

    // Left sidebar
    QListView *m_repoListView = nullptr;
    RepoListModel *m_repoListModel = nullptr;
    QPushButton *m_btnCheckout = nullptr;
    QPushButton *m_btnAddLocal = nullptr;
    QPushButton *m_btnSyncRecords = nullptr;
    QPushButton *m_btnSettings = nullptr;
    QPushButton *m_btnAbout = nullptr;

    // Main content
    QLabel *m_pathLabel = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QPushButton *m_goUpBtn = nullptr;
    QTableView *m_fileTableView = nullptr;
    QStandardItemModel *m_fileTableModel = nullptr;

    // Map: table row -> file path (from FileModel)
    QStringList m_currentFilePaths;

    // Status bar
    QLabel *m_statusRepoLabel = nullptr;
    QLabel *m_statusPathLabel = nullptr;
    QLabel *m_syncIndicator = nullptr;

    void refreshFileTable();

    QString m_currentPath; // kept for path-bar display sync; always use m_fileModel->currentPath() for actual loads
};
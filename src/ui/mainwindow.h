#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QListView>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QPropertyAnimation>

class ConfigService;
class FileModel;
class SyncEngine;
class RepoListModel;

namespace SVNFileBox {
class RepoGlobalManager;
}

// ── File card list delegate ────────────────────────────────────
class FileCardDelegate;

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

private slots:
    void onFileDoubleClicked(int row);
    void onRefreshClicked();
    void onGoUpClicked();
    void onPathEditingFinished();
    void onFilesChanged();
    void onSyncNotification(const QString &message);
    void onConflictDetected(const QStringList &files);
    void onSyncStarted();
    void onCopyProgress(int currentIndex, int totalCount, qint64 bytesCopied, qint64 totalBytes, const QString &currentFile);
    void onCopyCompleted(int copiedCount, int skippedCount, int overwrittenCount, const QString &errorMessage);

private:
    void setupUi();
    void setupLeftNav();
    void setupMiddlePanel();
    void setupRightPanel();
    void connectSignals();
    void navigateTo(const QString &path);
    void refreshFileTable();

    ConfigService *m_configService = nullptr;
    FileModel *m_fileModel = nullptr;
    SyncEngine *m_syncEngine = nullptr;
    SVNFileBox::RepoGlobalManager *m_globalManager = nullptr;

    // ── Left Navigation (48px) ─────────────────────────────────
    QWidget *m_navWidget = nullptr;
    QPushButton *m_navRepoBtn = nullptr;
    QPushButton *m_navSettingsBtn = nullptr;
    QPushButton *m_navAboutBtn = nullptr;
    QPushButton *m_navMinimizeBtn = nullptr;
    QPushButton *m_navCloseBtn = nullptr;

    // ── Middle Panel (260px) ────────────────────────────────────
    QWidget *m_middleWidget = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QListView *m_repoListView = nullptr;
    RepoListModel *m_repoListModel = nullptr;
    QPushButton *m_btnAddRepo = nullptr;
    QPushButton *m_btnAddLocal = nullptr;
    QPushButton *m_btnCheckout = nullptr;
    QPushButton *m_btnSyncRecords = nullptr;

    // ── Right Content Area ───────────────────────────────────────
    QWidget *m_rightWidget = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QPushButton *m_goUpBtn = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QListView *m_fileListView = nullptr;
    FileCardDelegate *m_fileDelegate = nullptr;

    // Status bar
    QLabel *m_statusRepoLabel = nullptr;
    QLabel *m_statusPathLabel = nullptr;
    QLabel *m_syncIndicator = nullptr;

    // File path mapping for card clicks
    QStringList m_currentFilePaths;
};
#include "mainwindow.h"
#include "repolistmodel.h"
#include "settingsdialog.h"
#include "checkoutdialog.h"
#include "addlocaldialog.h"
#include "copyprogressdialog.h"
#include "../config/configservice.h"
#include "../models/filemodel.h"
#include "../sync/syncengine.h"
#include "../services/repoglobalmanager.h"
#include "../services/repomanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QFileIconProvider>

MainWindow::MainWindow(ConfigService *configService,
                       FileModel *fileModel,
                       SyncEngine *syncEngine,
                       SVNFileBox::RepoGlobalManager *globalManager,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_configService(configService)
    , m_fileModel(fileModel)
    , m_syncEngine(syncEngine)
    , m_globalManager(globalManager)
{
    setupUi();
    connectSignals();

    // Load repos from config
    QVariantList repos = configService->repositories();
    m_repoListModel->loadFromConfig(repos);

    // Select the repo that was last active (isSelected == true)
    int selectedIndex = -1;
    for (int i = 0; i < repos.size(); ++i) {
        if (repos[i].toMap()["isSelected"].toBool()) {
            selectedIndex = i;
            break;
        }
    }
    if (selectedIndex == -1 && m_repoListModel->count() > 0) {
        selectedIndex = 0; // fallback to first repo
    }
    if (selectedIndex >= 0) {
        m_repoListModel->selectRepo(selectedIndex);
        QString path = m_repoListModel->repoPath(selectedIndex);
        if (!path.isEmpty()) {
            navigateTo(path);
            m_statusRepoLabel->setText(m_repoListModel->repoName(selectedIndex));
        }
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("SVNFileBox"));
    resize(1100, 700);

    // Central widget
    m_contentWidget = new QWidget(this);
    setCentralWidget(m_contentWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(m_contentWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Left sidebar
    QWidget *sidebar = new QWidget();
    sidebar->setFixedWidth(220);
    sidebar->setStyleSheet(QStringLiteral("QWidget { background: #FFFFFF; }"));
    mainLayout->addWidget(sidebar);

    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(12, 12, 12, 12);
    sidebarLayout->setSpacing(0);

    QLabel *repoListLabel = new QLabel(QStringLiteral("仓库列表"));
    repoListLabel->setStyleSheet(QStringLiteral("QLabel { font-size: 14px; font-weight: bold; color: #1A1A2E; margin-bottom: 8px; }"));
    sidebarLayout->addWidget(repoListLabel);

    m_repoListView = new QListView();
    m_repoListModel = new RepoListModel(this);
    m_repoListView->setModel(m_repoListModel);
    m_repoListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_repoListView->setSelectionMode(QAbstractItemView::SingleSelection);
    sidebarLayout->addWidget(m_repoListView);

    sidebarLayout->addSpacing(8);

    // Sidebar buttons
    QFrame *sep1 = new QFrame();
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet(QStringLiteral("QFrame { color: #E8E8E8; }"));
    sidebarLayout->addWidget(sep1);
    sidebarLayout->addSpacing(8);

    auto makeSidebarBtn = [](const QString &icon, const QString &text) {
        QPushButton *btn = new QPushButton(QStringLiteral("%1 %2").arg(icon).arg(text));
        btn->setFixedHeight(36);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { text-align: left; padding-left: 8px; "
            "background: #F0F0F0; border: none; border-radius: 4px; }"
            "QPushButton:hover { background: #E0E0E0; }"));
        return btn;
    };

    m_btnCheckout = makeSidebarBtn(QStringLiteral("🌐"), QStringLiteral("从网络添加仓库"));
    m_btnAddLocal = makeSidebarBtn(QStringLiteral("📂"), QStringLiteral("添加本地仓库"));
    m_btnSyncRecords = makeSidebarBtn(QStringLiteral("📋"), QStringLiteral("查看同步记录"));
    sidebarLayout->addWidget(m_btnCheckout);
    sidebarLayout->addWidget(m_btnAddLocal);
    sidebarLayout->addWidget(m_btnSyncRecords);

    QFrame *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet(QStringLiteral("QFrame { color: #E8E8E8; margin-top: 4px; margin-bottom: 4px; }"));
    sidebarLayout->addWidget(sep2);

    m_btnSettings = makeSidebarBtn(QStringLiteral("⚙️"), QStringLiteral("设置"));
    m_btnAbout = makeSidebarBtn(QStringLiteral("ℹ️"), QStringLiteral("关于"));
    sidebarLayout->addWidget(m_btnSettings);
    sidebarLayout->addWidget(m_btnAbout);
    sidebarLayout->addStretch();

    // Main content area
    QWidget *contentArea = new QWidget();
    contentArea->setStyleSheet(QStringLiteral("QWidget { background: #FFFFFF; }"));
    mainLayout->addWidget(contentArea,1);

    QVBoxLayout *contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // Path bar
    QWidget *pathBar = new QWidget();
    pathBar->setFixedHeight(48);
    pathBar->setStyleSheet(QStringLiteral("QWidget { background: #FAFBFC; border-bottom: 1px solid #E8E8E8; }"));
    contentLayout->addWidget(pathBar);

    QHBoxLayout *pathLayout = new QHBoxLayout(pathBar);
    pathLayout->setContentsMargins(12, 0, 12, 0);
    pathLayout->setSpacing(8);

    m_pathLabel = new QLabel(QStringLiteral("路径:"));
    m_pathLabel->setStyleSheet(QStringLiteral("QLabel { color: #666666; font-size: 12px; }"));
    pathLayout->addWidget(m_pathLabel);

    m_pathEdit = new QLineEdit();
    m_pathEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { border: none; background: transparent; font-size: 13px; color: #333333; }"));
    m_pathEdit->setReadOnly(false);
    pathLayout->addWidget(m_pathEdit, 1);

    m_refreshBtn = new QPushButton(QStringLiteral("刷新"));
    m_refreshBtn->setFixedSize(70, 32);
    m_refreshBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #1565C0; }"));

    m_goUpBtn = new QPushButton(QStringLiteral("↑ 返回"));
    m_goUpBtn->setFixedSize(70, 32);
    m_goUpBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #F5F5F5; color: #333; border: 1px solid #E0E0E0; border-radius: 4px; }"
        "QPushButton:hover { background: #E0E0E0; }"));

    pathLayout->addWidget(m_goUpBtn);
    pathLayout->addWidget(m_refreshBtn);

    // File table
    m_fileTableModel = new QStandardItemModel(this);
    m_fileTableModel->setHorizontalHeaderLabels({
        QStringLiteral("类型"), QStringLiteral("名称"), QStringLiteral("状态"),
        QStringLiteral("大小"), QStringLiteral("修改时间"), QStringLiteral("文件类型")
    });

    m_fileTableView = new QTableView();
    m_fileTableView->setModel(m_fileTableModel);
    m_fileTableView->setAlternatingRowColors(true);
    m_fileTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileTableView->verticalHeader()->setVisible(false);

    // Column widths
    m_fileTableView->setColumnWidth(0, 40);
    m_fileTableView->setColumnWidth(1, 280);
    m_fileTableView->setColumnWidth(2, 70);
    m_fileTableView->setColumnWidth(3, 90);
    m_fileTableView->setColumnWidth(4, 140);
    m_fileTableView->setColumnWidth(5, 80);

    contentLayout->addWidget(m_fileTableView, 1);

    // Status bar
    QStatusBar *statusBar = new QStatusBar();
    statusBar->setFixedHeight(36);
    statusBar->setStyleSheet(QStringLiteral(
        "QStatusBar { background: #1E88E5; color: white; }"));
    setStatusBar(statusBar);

    m_statusRepoLabel = new QLabel();
    m_statusRepoLabel->setStyleSheet(QStringLiteral(
        "QLabel { background: #1565C0; color: white; padding: 2px 6px; "
        "border-radius: 3px; font-size: 11px; }"));
    statusBar->addPermanentWidget(m_statusRepoLabel);

    m_statusPathLabel = new QLabel();
    m_statusPathLabel->setStyleSheet(QStringLiteral("QLabel { color: white; font-size: 12px; }"));
    statusBar->addWidget(m_statusPathLabel);

    m_syncIndicator = new QLabel(QStringLiteral("●"));
    m_syncIndicator->setStyleSheet(QStringLiteral("QLabel { color: white; font-size: 10px; }"));
    statusBar->addPermanentWidget(m_syncIndicator);
}

void MainWindow::connectSignals()
{
    connect(m_repoListView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::onRepoSelected);

    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(m_goUpBtn, &QPushButton::clicked, this, &MainWindow::onGoUpClicked);

    connect(m_pathEdit, &QLineEdit::editingFinished,
            this, &MainWindow::onPathEditingFinished);

    connect(m_fileModel, &FileModel::currentPathChanged,
            this, &MainWindow::onFilesChanged);

    connect(m_fileModel, &FileModel::copyProgress,
            this, &MainWindow::onCopyProgress);
    connect(m_fileModel, &FileModel::copyCompleted,
            this, &MainWindow::onCopyCompleted);

    connect(m_syncEngine, &SyncEngine::syncNotification,
            this, &MainWindow::onSyncNotification);
    connect(m_syncEngine, &SyncEngine::filesChanged,
            this, &MainWindow::onFilesChanged);
    connect(m_syncEngine, &SyncEngine::conflictDetected,
            this, &MainWindow::onConflictDetected);
    connect(m_syncEngine, &SyncEngine::syncStarted,
            this, &MainWindow::onSyncStarted);

    connect(m_globalManager, &SVNFileBox::RepoGlobalManager::filesChanged,
            this, &MainWindow::onFilesChanged);
    connect(m_globalManager, &SVNFileBox::RepoGlobalManager::syncNotification,
            this, &MainWindow::onSyncNotification);
    connect(m_globalManager, &SVNFileBox::RepoGlobalManager::conflictDetected,
            this, &MainWindow::onConflictDetected);

    // File table double-click
    connect(m_fileTableView, &QTableView::doubleClicked,
            this, &MainWindow::onFileDoubleClicked);

    // Sidebar buttons
    connect(m_btnCheckout, &QPushButton::clicked, this, [this]() {
        CheckoutDialog dlg(m_globalManager, this);
        dlg.exec();
    });
    connect(m_btnAddLocal, &QPushButton::clicked, this, [this]() {
        AddLocalDialog dlg(m_globalManager, m_configService, this);
        if (dlg.exec() == QDialog::Accepted) {
            // Refresh repo list from config
            QVariantList repos = m_configService->repositories();
            m_repoListModel->loadFromConfig(repos);
            // Switch to the newly added repo (last one)
            if (!repos.isEmpty()) {
                int last = repos.size() - 1;
                m_repoListModel->selectRepo(last);
                QString path = m_repoListModel->repoPath(last);
                if (!path.isEmpty()) {
                    navigateTo(path);
                    m_statusRepoLabel->setText(m_repoListModel->repoName(last));
                }
            }
        }
    });
    connect(m_btnSyncRecords, &QPushButton::clicked, this, [this]() {
        SyncRecordsDialog dlg(this);
        dlg.exec();
    });
    connect(m_btnSettings, &QPushButton::clicked, this, [this]() {
        SettingsDialog dlg(m_configService, this);
        dlg.exec();
    });
    connect(m_btnAbout, &QPushButton::clicked, this, [this]() {
        AboutDialog dlg(this);
        dlg.exec();
    });
}

void MainWindow::onRepoSelected(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int row = index.row();
    m_repoListModel->selectRepo(row);
    QString path = m_repoListModel->repoPath(row);
    if (!path.isEmpty()) {
        navigateTo(path);
        m_statusRepoLabel->setText(m_repoListModel->repoName(row));
    }
}

void MainWindow::onFileDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int row = index.row();
    if (row < 0 || row >= m_currentFilePaths.size()) return;

    QString path = m_currentFilePaths[row];
    if (path.isEmpty()) return;

    QFileInfo info(path);
    if (info.isDir()) {
        navigateTo(path);
    }
    // TODO: open file
}

void MainWindow::onRefreshClicked()
{
    m_fileModel->load(m_currentPath);
}

void MainWindow::onPathEditingFinished()
{
    QString newPath = m_pathEdit->text().trimmed();
    if (!newPath.isEmpty() && newPath != m_currentPath) {
        navigateTo(newPath);
    }
}

void MainWindow::onManualSync()
{
    if (m_globalManager->activeManager()) {
        m_globalManager->enqueueCommit(m_currentPath, 0);
    }
}

void MainWindow::onGoUpClicked()
{
    goUp();
}

void MainWindow::navigateTo(const QString &path)
{
    m_currentPath = path;
    m_pathEdit->setText(path);
    m_statusPathLabel->setText(path);
    m_fileModel->load(path);
    // currentPathChanged is now emitted AFTER load() fills the model,
    // so onFilesChanged -> refreshFileTable() will show the correct data.
    // Direct call below as fallback / for non-signal triggers.
    refreshFileTable();
}

void MainWindow::goUp()
{
    QFileInfo info(m_currentPath);
    QString parent = info.dir().absolutePath();
    if (parent != m_currentPath) {
        navigateTo(parent);
    }
}

void MainWindow::refreshFileTable()
{
    m_fileTableModel->removeRows(0, m_fileTableModel->rowCount());
    m_currentFilePaths.clear();

    for (int i = 0; i < m_fileModel->rowCount(); ++i) {
        QVariantMap item = m_fileModel->get(i);
        if (item.isEmpty()) continue;

        QString name = item["name"].toString();
        QString fullPath = item["path"].toString();
        QString svnStatus = item["svnStatus"].toString();
        QString size = item["size"].toString();
        QString modifiedTime = item["lastModified"].toString();
        QString fileType = item["isDir"].toBool() ? QStringLiteral("文件夹") : QFileInfo(name).suffix();

        m_currentFilePaths.append(fullPath);

        QList<QStandardItem *> row;

        // Column 0: icon (text emoji placeholder)
        QStandardItem *iconCol = new QStandardItem();
        iconCol->setData(item["isDir"].toBool() ? QStringLiteral("📁") : QStringLiteral("📄"), Qt::DisplayRole);
        iconCol->setData(fullPath, Qt::UserRole);
        row.append(iconCol);

        // Column 1: name
        QStandardItem *nameCol = new QStandardItem(name);
        nameCol->setData(fullPath, Qt::UserRole);
        row.append(nameCol);

        // Column 2: svn status
        QStandardItem *statusCol = new QStandardItem(svnStatus);
        statusCol->setData(fullPath, Qt::UserRole);
        row.append(statusCol);

        // Column 3: size
        QStandardItem *sizeCol = new QStandardItem(size);
        sizeCol->setData(fullPath, Qt::UserRole);
        row.append(sizeCol);

        // Column 4: modified time
        QStandardItem *timeCol = new QStandardItem(modifiedTime);
        timeCol->setData(fullPath, Qt::UserRole);
        row.append(timeCol);

        // Column 5: file type
        QStandardItem *typeCol = new QStandardItem(fileType);
        typeCol->setData(fullPath, Qt::UserRole);
        row.append(typeCol);

        m_fileTableModel->appendRow(row);
    }
}

void MainWindow::onFilesChanged()
{
    refreshFileTable();
}

void MainWindow::onSyncNotification(const QString &message)
{
    m_statusPathLabel->setText(message);
}

void MainWindow::onConflictDetected(const QStringList &files)
{
    // TODO: show conflict dialog
    QMessageBox::warning(this, QStringLiteral("冲突"),
        QStringLiteral("检测到冲突文件: %1").arg(files.join(", ")));
}

void MainWindow::onSyncStarted()
{
    m_syncIndicator->setStyleSheet(QStringLiteral(
        "QLabel { color: #BBDEFB; font-size: 10px; animation: pulse 1s infinite; }"));
}

void MainWindow::onCopyProgress(int currentIndex, int totalCount,
                                qint64 bytesCopied, qint64 totalBytes,
                                const QString &currentFile)
{
    // TODO: show copy progress dialog
    Q_UNUSED(currentIndex);
    Q_UNUSED(totalCount);
    Q_UNUSED(bytesCopied);
    Q_UNUSED(totalBytes);
    m_statusPathLabel->setText(QStringLiteral("复制中: %1").arg(currentFile));
}

void MainWindow::onCopyCompleted(int copiedCount, int skippedCount,
                                 int overwrittenCount, const QString &errorMessage)
{
    // TODO: close copy progress dialog
    if (!errorMessage.isEmpty()) {
        m_statusPathLabel->setText(QStringLiteral("导入失败: %1").arg(errorMessage));
    } else {
        m_statusPathLabel->setText(QStringLiteral("已导入 %1 项，跳过 %2 项，覆盖 %3 项")
            .arg(copiedCount).arg(skippedCount).arg(overwrittenCount));
    }
    refreshFileTable();
}
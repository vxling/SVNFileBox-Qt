#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "repolistmodel.h"
#include "repoitemdelegate.h"
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
#include <QFileInfo>

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
    ui = new Ui::MainWindow;
    ui->setupUi(this);
    setupUiFromCode();
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

void MainWindow::setupUiFromCode()
{
    // Grab pointers from UI
    m_repoListView = ui->m_repoListView;
    m_btnCheckout = ui->m_btnCheckout;
    m_btnAddLocal = ui->m_btnAddLocal;
    m_btnSyncRecords = ui->m_btnSyncRecords;
    m_btnSettings = ui->m_btnSettings;
    m_btnAbout = ui->m_btnAbout;

    m_pathLabel = ui->m_pathLabel;
    m_pathEdit = ui->m_pathEdit;
    m_goUpBtn = ui->m_goUpBtn;
    m_refreshBtn = ui->m_refreshBtn;
    m_fileTableView = ui->m_fileTableView;

    // Set up repo list model
    m_repoListModel = new RepoListModel(this);
    m_repoListView->setModel(m_repoListModel);
    m_repoListView->setItemDelegate(new RepoItemDelegate(this));
    m_repoListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Set up file table model
    m_fileTableModel = new QStandardItemModel(this);
    m_fileTableModel->setHorizontalHeaderLabels({
        QStringLiteral("类型"), QStringLiteral("名称"), QStringLiteral("状态"),
        QStringLiteral("大小"), QStringLiteral("修改时间"), QStringLiteral("文件类型")
    });
    m_fileTableView->setModel(m_fileTableModel);

    // Column widths
    m_fileTableView->setColumnWidth(0, 40);
    m_fileTableView->setColumnWidth(1, 280);
    m_fileTableView->setColumnWidth(2, 70);
    m_fileTableView->setColumnWidth(3, 90);
    m_fileTableView->setColumnWidth(4, 140);
    m_fileTableView->setColumnWidth(5, 80);
    m_fileTableView->verticalHeader()->setVisible(false);

    // File table style: WPF look
    m_fileTableView->setAlternatingRowColors(false);
    m_fileTableView->setShowGrid(false);
    m_fileTableView->setStyleSheet(QStringLiteral(
        "QTableView {"
        "  background: #FFFFFF; border: none; outline: none; }"
        "QTableView::item {"
        "  background: #FFFFFF; padding: 0 10px; }"
        "QTableView::item:selected {"
        "  background: #E1F0FE; color: #333333; }"
        "QTableView::item:hover {"
        "  background: #F2F8FF; }"
        "QHeaderView::section {"
        "  background: #F2F2F2; color: #999999; padding: 0 10px; "
        "  border: none; border-right: 1px solid #D1D1D1; font: 12px \"Microsoft YaHei UI\"; }"
        "QHeaderView::section:last { border-right: none; }"
        "QScrollBar:vertical { background: transparent; width: 6px; }"
        "QScrollBar::handle:vertical { background: #D0D0D0; border-radius: 3px; }")
    );
    // Row height approx 32px


    // Status bar widgets
    QStatusBar *statusBar = ui->statusbar;

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
            QVariantList repos = m_configService->repositories();
            m_repoListModel->loadFromConfig(repos);
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
}

void MainWindow::onRefreshClicked()
{
    QString path = m_fileModel->currentPath();
    if (path.isEmpty()) {
        auto *active = m_globalManager->activeManager();
        if (active) path = active->repository.path;
    }
    if (!path.isEmpty()) {
        m_fileModel->load(path);
    }
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

        QStandardItem *iconCol = new QStandardItem();
        iconCol->setData(item["isDir"].toBool() ? QStringLiteral("📁") : QStringLiteral("📄"), Qt::DisplayRole);
        iconCol->setData(fullPath, Qt::UserRole);
        row.append(iconCol);

        QStandardItem *nameCol = new QStandardItem(name);
        nameCol->setData(fullPath, Qt::UserRole);
        row.append(nameCol);

        QStandardItem *statusCol = new QStandardItem(svnStatus);
        statusCol->setData(fullPath, Qt::UserRole);
        row.append(statusCol);

        QStandardItem *sizeCol = new QStandardItem(size);
        sizeCol->setData(fullPath, Qt::UserRole);
        row.append(sizeCol);

        QStandardItem *timeCol = new QStandardItem(modifiedTime);
        timeCol->setData(fullPath, Qt::UserRole);
        row.append(timeCol);

        QStandardItem *typeCol = new QStandardItem(fileType);
        typeCol->setData(fullPath, Qt::UserRole);
        row.append(typeCol);

        m_fileTableModel->appendRow(row);
    }
}

void MainWindow::onFilesChanged()
{
    m_currentPath = m_fileModel->currentPath();
    refreshFileTable();
}

void MainWindow::onSyncNotification(const QString &message)
{
    m_statusPathLabel->setText(message);
}

void MainWindow::onConflictDetected(const QStringList &files)
{
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
    Q_UNUSED(currentIndex);
    Q_UNUSED(totalCount);
    Q_UNUSED(bytesCopied);
    Q_UNUSED(totalBytes);
    m_statusPathLabel->setText(QStringLiteral("复制中: %1").arg(currentFile));
}

void MainWindow::onCopyCompleted(int copiedCount, int skippedCount,
                                 int overwrittenCount, const QString &errorMessage)
{
    if (!errorMessage.isEmpty()) {
        m_statusPathLabel->setText(QStringLiteral("导入失败: %1").arg(errorMessage));
    } else {
        m_statusPathLabel->setText(QStringLiteral("已导入 %1 项，跳过 %2 项，覆盖 %3 项")
            .arg(copiedCount).arg(skippedCount).arg(overwrittenCount));
    }
    refreshFileTable();
}
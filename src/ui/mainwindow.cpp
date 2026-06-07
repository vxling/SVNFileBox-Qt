#include "mainwindow.h"
#include "repolistmodel.h"
#include "filecarddelegate.h"
#include "settingsdialog.h"
#include "checkoutdialog.h"
#include "addlocaldialog.h"
#include "copyprogressdialog.h"
#include "newfiledialog.h"
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
#include <QToolButton>
#include <QStatusBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainterPath>
#include <QFileInfo>

// ── WeChat green palette ───────────────────────────────────────
static const QColor WX_GREEN(0x07, 0xC1, 0x60);
static const QColor WX_LIGHT_GREEN(0xDC, 0xF5, 0xE8);
static const QColor WX_BG_GRAY(0xF2, 0xF2, 0xF5);
static const QColor WX_TEXT_DARK(0x33, 0x33, 0x33);
static const QColor WX_TEXT_SEC(0x66, 0x66, 0x66);
static const QColor WX_TEXT_LIGHT(0xB2, 0xB2, 0xB2);
static const QColor WX_BORDER(0xEE, 0xEE, 0xEE);
static const QColor WX_WHITE(0xFF, 0xFF, 0xFF);

static QString wxBtnStyle(bool selected = false) {
    if (selected) {
        return QStringLiteral(
            "QPushButton { background: transparent; border: none; border-radius: 8px; }"
            "QPushButton:hover { background: #E8F9EE; }");
    }
    return QStringLiteral(
        "QPushButton { background: transparent; border: none; border-radius: 8px; }"
        "QPushButton:hover { background: #F0F0F0; }");
}

// ── Constructor ────────────────────────────────────────────────
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
    QVariantList repos = m_configService->repositories();
    m_repoListModel->loadFromConfig(repos);

    // Restore last active repo
    int selectedIndex = -1;
    for (int i = 0; i < repos.size(); ++i) {
        if (repos[i].toMap()["isSelected"].toBool()) {
            selectedIndex = i;
            break;
        }
    }
    if (selectedIndex == -1 && m_repoListModel->count() > 0) selectedIndex = 0;
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

// ── Setup UI ───────────────────────────────────────────────────
void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("SVNFileBox"));
    resize(1200, 720);
    setMinimumSize(900, 540);

    // Remove window title bar — we use custom layout
    setWindowFlags(Qt::FramelessWindowHint);

    QWidget *root = new QWidget(this);
    root->setStyleSheet(QStringLiteral("QWidget { background: %1; }").arg(WX_BG_GRAY.name()));
    setCentralWidget(root);

    QHBoxLayout *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    setupLeftNav();
    setupMiddlePanel();
    setupRightPanel();

    rootLayout->addWidget(m_navWidget);       // 48px
    rootLayout->addWidget(m_middleWidget);    // 260px
    rootLayout->addWidget(m_rightWidget, 1);  // flex

    // Status bar
    QStatusBar *statusBar = new QStatusBar();
    statusBar->setFixedHeight(28);
    statusBar->setStyleSheet(QStringLiteral(
        "QStatusBar { background: #1A1A1A; color: #CCCCCC; border-top: 1px solid #333; }"
        "QStatusBar::item { border: none; }"));
    setStatusBar(statusBar);

    m_statusRepoLabel = new QLabel();
    m_statusRepoLabel->setStyleSheet(QStringLiteral(
        "QLabel { background: %1; color: white; padding: 1px 6px; "
        "border-radius: 3px; font-size: 10px; }").arg(WX_GREEN.name()));
    statusBar->addPermanentWidget(m_statusRepoLabel);

    m_statusPathLabel = new QLabel();
    m_statusPathLabel->setStyleSheet(QStringLiteral("QLabel { color: #AAAAAA; font-size: 11px; }"));
    statusBar->addWidget(m_statusPathLabel);

    m_syncIndicator = new QLabel(QStringLiteral("●"));
    m_syncIndicator->setStyleSheet(QStringLiteral("QLabel { color: %1; font-size: 10px; }").arg(WX_GREEN.name()));
    statusBar->addPermanentWidget(m_syncIndicator);
}

void MainWindow::setupLeftNav()
{
    m_navWidget = new QWidget();
    m_navWidget->setFixedWidth(48);
    m_navWidget->setStyleSheet(QStringLiteral("QWidget { background: #2A2A2A; }"));

    QVBoxLayout *layout = new QVBoxLayout(m_navWidget);
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(4);

    // User avatar at top
    QLabel *avatar = new QLabel(QStringLiteral("U"));
    avatar->setFixedSize(36, 36);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet(QStringLiteral(
        "QLabel { background: %1; color: white; border-radius: 18px; "
        "font-size: 16px; font-weight: bold; }").arg(WX_GREEN.name()));
    avatar->setMargin(0);
    layout->addWidget(avatar, 0, Qt::AlignHCenter);

    layout->addSpacing(12);

    // Nav buttons
    auto navBtn = [this](QPushButton *&btn, const QString &icon, const QString &tooltip) {
        btn = new QPushButton(QStringLiteral("%1").arg(icon));
        btn->setFixedSize(36, 36);
        btn->setToolTip(tooltip);
        btn->setCursor(Qt::PointingHandCursor);
    };

    navBtn(m_navRepoBtn, QStringLiteral("📂"), QStringLiteral("仓库"));
    navBtn(m_navSettingsBtn, QStringLiteral("⚙"), QStringLiteral("设置"));
    navBtn(m_navAboutBtn, QStringLiteral("ℹ"), QStringLiteral("关于"));
    navBtn(m_navMinimizeBtn, QStringLiteral("─"), QStringLiteral("最小化"));
    navBtn(m_navCloseBtn, QStringLiteral("✕"), QStringLiteral("关闭"));

    m_navRepoBtn->setStyleSheet(wxBtnStyle(true));
    m_navSettingsBtn->setStyleSheet(wxBtnStyle());
    m_navAboutBtn->setStyleSheet(wxBtnStyle());
    m_navMinimizeBtn->setStyleSheet(wxBtnStyle());
    m_navCloseBtn->setStyleSheet(wxBtnStyle());

    // Colorize close button hover
    m_navCloseBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; border-radius: 8px; color: #888; }"
        "QPushButton:hover { background: #E53935; color: white; }"));

    // Bottom: settings at bottom
    layout->addStretch();
    layout->addWidget(m_navRepoBtn, 0, Qt::AlignHCenter);
    layout->addSpacing(4);
    layout->addWidget(m_navSettingsBtn, 0, Qt::AlignHCenter);
    layout->addWidget(m_navAboutBtn, 0, Qt::AlignHCenter);
    layout->addSpacing(4);
    layout->addWidget(m_navMinimizeBtn, 0, Qt::AlignHCenter);
    layout->addWidget(m_navCloseBtn, 0, Qt::AlignHCenter);
}

void MainWindow::setupMiddlePanel()
{
    m_middleWidget = new QWidget();
    m_middleWidget->setFixedWidth(260);
    m_middleWidget->setStyleSheet(QStringLiteral("QWidget { background: %1; }").arg(WX_WHITE.name()));

    QVBoxLayout *layout = new QVBoxLayout(m_middleWidget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    // ── Search box ──────────────────────────────────────────────
    m_searchInput = new QLineEdit();
    m_searchInput->setFixedHeight(32);
    m_searchInput->setPlaceholderText(QStringLiteral("搜索仓库..."));
    m_searchInput->setStyleSheet(QStringLiteral(
        "QLineEdit { background: %1; border: none; border-radius: 16px; "
        "padding: 0 16px; font-size: 13px; color: %2; }"
        "QLineEdit:focus { background: white; border: 1px solid %3; }"
        "QLineEdit::placeholder { color: %4; }")
        .arg(WX_BG_GRAY.name()).arg(WX_TEXT_DARK.name())
        .arg(WX_GREEN.name()).arg(WX_TEXT_LIGHT.name()));
    layout->addWidget(m_searchInput);

    // ── Repo list ───────────────────────────────────────────────
    m_repoListModel = new RepoListModel(this);
    m_repoListView = new QListView();
    m_repoListView->setModel(m_repoListModel);
    m_repoListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_repoListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_repoListView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_repoListView->setSpacing(2);
    m_repoListView->setStyleSheet(QStringLiteral(
        "QListView { background: transparent; border: none; outline: none; }"
        "QListView::item { padding: 0; margin: 0; }"
        "QListView::item:selected { background: %1; color: white; border-radius: 6px; }"
        "QListView::item:selected:!active { background: %1; color: white; border-radius: 6px; }"
        "QListView::item:hover:!selected { background: %2; border-radius: 6px; }")
        .arg(WX_GREEN.name()).arg(WX_LIGHT_GREEN.name()));

    layout->addWidget(m_repoListView, 1);

    // ── Action buttons ──────────────────────────────────────────
    auto makeActionBtn = [](const QString &text, const QString &icon) {
        QPushButton *btn = new QPushButton(QStringLiteral("%1 %2").arg(icon).arg(text));
        btn->setFixedHeight(34);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { text-align: left; padding-left: 12px; background: %1; "
            "border: none; border-radius: 6px; font-size: 12px; color: %2; }"
            "QPushButton:hover { background: %3; }")
            .arg(WX_BG_GRAY.name()).arg(WX_TEXT_DARK.name())
            .arg(WX_LIGHT_GREEN.name()));
        return btn;
    };

    m_btnAddLocal = makeActionBtn(QStringLiteral("添加本地仓库"), QStringLiteral("📂"));
    m_btnCheckout = makeActionBtn(QStringLiteral("从网络添加"), QStringLiteral("🌐"));
    m_btnSyncRecords = makeActionBtn(QStringLiteral("同步记录"), QStringLiteral("📋"));

    QVBoxLayout *actionLayout = new QVBoxLayout();
    actionLayout->setSpacing(4);
    actionLayout->addWidget(m_btnAddLocal);
    actionLayout->addWidget(m_btnCheckout);
    actionLayout->addWidget(m_btnSyncRecords);
    layout->addLayout(actionLayout);
}

void MainWindow::setupRightPanel()
{
    m_rightWidget = new QWidget();
    m_rightWidget->setStyleSheet(QStringLiteral("QWidget { background: %1; }").arg(WX_BG_GRAY.name()));

    QVBoxLayout *layout = new QVBoxLayout(m_rightWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Top bar ─────────────────────────────────────────────────
    QWidget *topBar = new QWidget();
    topBar->setFixedHeight(52);
    topBar->setStyleSheet(QStringLiteral("QWidget { background: white; border-bottom: 1px solid %1; }").arg(WX_BORDER.name()));

    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 0, 16, 0);
    topLayout->setSpacing(8);

    m_goUpBtn = new QPushButton(QStringLiteral("↑"));
    m_goUpBtn->setFixedSize(32, 32);
    m_goUpBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: none; border-radius: 6px; "
        "font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background: %3; }")
        .arg(WX_BG_GRAY.name()).arg(WX_TEXT_DARK.name()).arg(WX_LIGHT_GREEN.name()));

    m_pathEdit = new QLineEdit();
    m_pathEdit->setFixedHeight(34);
    m_pathEdit->setPlaceholderText(QStringLiteral("输入路径后回车..."));
    m_pathEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background: %1; border: none; border-radius: 6px; "
        "padding: 0 12px; font-size: 13px; color: %2; }"
        "QLineEdit:focus { background: white; border: 1px solid %3; border-radius: 6px; }"
        "QLineEdit::placeholder { color: %4; }")
        .arg(WX_BG_GRAY.name()).arg(WX_TEXT_DARK.name())
        .arg(WX_GREEN.name()).arg(WX_TEXT_LIGHT.name()));

    m_refreshBtn = new QPushButton(QStringLiteral("🔄"));
    m_refreshBtn->setFixedSize(34, 34);
    m_refreshBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; border-radius: 6px; font-size: 15px; }"
        "QPushButton:hover { background: %1; }").arg(WX_LIGHT_GREEN.name()));

    topLayout->addWidget(m_goUpBtn);
    topLayout->addWidget(m_pathEdit, 1);
    topLayout->addWidget(m_refreshBtn);
    layout->addWidget(topBar);

    // ── File card list ──────────────────────────────────────────
    m_fileListView = new QListView();
    m_fileListView->setAlternatingRowColors(false);
    m_fileListView->setSpacing(6);
    m_fileListView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_fileListView->setStyleSheet(QStringLiteral(
        "QListView { background: %1; border: none; outline: none; padding: 8px 12px; }"
        "QListView::item { padding: 0; margin: 0; border: none; }"
        "QListView::item:selected { background: transparent; }"
        "QListView::item:hover:!selected { background: transparent; }")
        .arg(WX_BG_GRAY.name()));

    layout->addWidget(m_fileListView, 1);
}

// ── Connect signals ─────────────────────────────────────────────
void MainWindow::connectSignals()
{
    // Nav buttons
    connect(m_navMinimizeBtn, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(m_navCloseBtn, &QPushButton::clicked, this, &QWidget::close);
    connect(m_navAboutBtn, &QPushButton::clicked, this, [this]() {
        AboutDialog dlg(this);
        dlg.exec();
    });
    connect(m_navSettingsBtn, &QPushButton::clicked, this, [this]() {
        SettingsDialog dlg(m_configService, this);
        dlg.exec();
    });

    // Middle panel buttons
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
    connect(m_btnCheckout, &QPushButton::clicked, this, [this]() {
        CheckoutDialog dlg(m_globalManager, this);
        dlg.exec();
    });
    connect(m_btnSyncRecords, &QPushButton::clicked, this, [this]() {
        SyncRecordsDialog dlg(this);
        dlg.exec();
    });

    // Repo list selection
    connect(m_repoListView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &index) {
                if (!index.isValid()) return;
                int row = index.row();
                m_repoListModel->selectRepo(row);
                QString path = m_repoListModel->repoPath(row);
                if (!path.isEmpty()) {
                    navigateTo(path);
                    m_statusRepoLabel->setText(m_repoListModel->repoName(row));
                }
            });

    // Top bar
    connect(m_goUpBtn, &QPushButton::clicked, this, &MainWindow::onGoUpClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(m_pathEdit, &QLineEdit::editingFinished, this, &MainWindow::onPathEditingFinished);

    // File model signals
    connect(m_fileModel, &FileModel::currentPathChanged, this, &MainWindow::onFilesChanged);
    connect(m_fileModel, &FileModel::copyProgress, this, &MainWindow::onCopyProgress);
    connect(m_fileModel, &FileModel::copyCompleted, this, &MainWindow::onCopyCompleted);

    // Sync engine
    connect(m_syncEngine, &SyncEngine::syncNotification, this, &MainWindow::onSyncNotification);
    connect(m_syncEngine, &SyncEngine::filesChanged, this, &MainWindow::onFilesChanged);
    connect(m_syncEngine, &SyncEngine::conflictDetected, this, &MainWindow::onConflictDetected);
    connect(m_syncEngine, &SyncEngine::syncStarted, this, &MainWindow::onSyncStarted);

    // Global manager
    connect(m_globalManager, &SVNFileBox::RepoGlobalManager::filesChanged,
            this, &MainWindow::onFilesChanged);
    connect(m_globalManager, &SVNFileBox::RepoGlobalManager::syncNotification,
            this, &MainWindow::onSyncNotification);
    connect(m_globalManager, &SVNFileBox::RepoGlobalManager::conflictDetected,
            this, &MainWindow::onConflictDetected);

    // File card double-click
    connect(m_fileListView, &QListView::doubleClicked, this, [this](const QModelIndex &index) {
        onFileDoubleClicked(index.row());
    });
}

// ── Navigation ────────────────────────────────────────────────
void MainWindow::navigateTo(const QString &path)
{
    m_pathEdit->setText(path);
    m_fileModel->load(path);
    refreshFileTable();
    m_statusPathLabel->setText(path);
}

void MainWindow::onFileDoubleClicked(int row)
{
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
    if (!path.isEmpty()) m_fileModel->load(path);
}

void MainWindow::onGoUpClicked()
{
    QString path = m_fileModel->currentPath();
    if (path.isEmpty()) return;
    QFileInfo info(path);
    QString parent = info.dir().absolutePath();
    if (parent != path) navigateTo(parent);
}

void MainWindow::onPathEditingFinished()
{
    QString newPath = m_pathEdit->text().trimmed();
    if (!newPath.isEmpty() && newPath != m_fileModel->currentPath()) {
        navigateTo(newPath);
    }
}

void MainWindow::onFilesChanged()
{
    refreshFileTable();
}

// ── Refresh file card list ─────────────────────────────────────
void MainWindow::refreshFileTable()
{
    // Build a QStandardItemModel for the card list
    static QStandardItemModel *cardModel = nullptr;
    if (!cardModel) {
        cardModel = new QStandardItemModel(this);
        m_fileListView->setModel(cardModel);
        m_fileDelegate = new FileCardDelegate(this);
        m_fileListView->setItemDelegate(m_fileDelegate);
    }

    cardModel->removeRows(0, cardModel->rowCount());
    m_currentFilePaths.clear();

    for (int i = 0; i < m_fileModel->rowCount(); ++i) {
        QVariantMap item = m_fileModel->get(i);
        if (item.isEmpty()) continue;

        QString name = item["name"].toString();
        QString fullPath = item["path"].toString();
        bool isDir = item["isDir"].toBool();
        QString svnStatus = item["svnStatus"].toString();
        QString size = item["size"].toString();
        QString modifiedTime = item["lastModified"].toString();

        m_currentFilePaths.append(fullPath);

        QStandardItem *row = new QStandardItem(name);
        row->setData(isDir ? QStringLiteral("true") : QStringLiteral("false"), Qt::UserRole + 1);
        row->setData(svnStatus, Qt::UserRole + 2);
        row->setData(size, Qt::UserRole + 3);
        row->setData(modifiedTime, Qt::UserRole + 4);
        row->setData(fullPath, Qt::UserRole + 5);
        cardModel->appendRow(row);
    }
}

void MainWindow::onSyncNotification(const QString &message)
{
    m_statusPathLabel->setText(message);
}

void MainWindow::onConflictDetected(const QStringList &files)
{
    ConflictDialog dlg(files, this);
    dlg.exec();
}

void MainWindow::onSyncStarted()
{
    m_syncIndicator->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 10px; animation: pulse 1s infinite; }").arg(WX_GREEN.name()));
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
        m_statusPathLabel->setText(QStringLiteral("已导入 %1 项 | 跳过 %2 项 | 覆盖 %3 项")
            .arg(copiedCount).arg(skippedCount).arg(overwrittenCount));
    }
}
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
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QMenu>
#include <QClipboard>
#include <QIcon>
#include <QProcess>
#include <QMimeData>
#include <QApplication>
#include <QDirIterator>
#include <zip.h>
static bool copyDirectory(const QString &srcPath, const QString &destPath);

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
    m_fileTableView->setContextMenuPolicy(Qt::CustomContextMenu);

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
    connect(m_globalManager, &SVNFileBox::RepoGlobalManager::repositoryFocused,
            this, &MainWindow::onRepositoryFocused);

    // File table double-click
    connect(m_fileTableView, &QTableView::doubleClicked,
            this, &MainWindow::onFileDoubleClicked);
    connect(m_fileTableView, &QTableView::customContextMenuRequested,
            this, &MainWindow::onFileContextMenu);

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
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MainWindow::onFileContextMenu(const QPoint &pos)
{
    QModelIndex index = m_fileTableView->indexAt(pos);
    QString selectedPath;
    bool isDir = false;
    if (index.isValid() && index.row() < m_currentFilePaths.size()) {
        selectedPath = m_currentFilePaths[index.row()];
        isDir = QFileInfo(selectedPath).isDir();
    }

    QMenu menu;

    // 打开 (open file/folder with default app)
    QAction *openAct = menu.addAction(QStringLiteral("\xe6\x89\x93\xe5\xbc\x80"));
    menu.addSeparator();

    // 刷新
    QAction *refreshAct = menu.addAction(QStringLiteral("\xe5\x88\xb7\xe6\x96\xb0"));
    menu.addSeparator();

    // 复制
    QAction *copyAct = menu.addAction(QStringLiteral("\xe5\xa4\x8d\xe5\x88\xb6"));
    // 粘贴
    QAction *pasteAct = menu.addAction(QStringLiteral("\xe7\xb2\x98\xe8\xb4\xb4"));
    menu.addSeparator();

    // 删除
    QAction *deleteAct = menu.addAction(QStringLiteral("\xe5\x88\xa0\xe9\x99\xa4"));
    menu.addSeparator();

    // 重命名
    QAction *renameAct = menu.addAction(QStringLiteral("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d"));
    menu.addSeparator();

    // 添加到 .Zip 压缩包
    QAction *compressAct = menu.addAction(QStringLiteral("\xe6\xb7\xbb\xe5\x8a\xa0\xe5\x88\xb0 .Zip \xe5\x8e\x8b\xe7\xbc\xa9\xe5\x8c\x85"));
    menu.addSeparator();

    // 新建 (submenu)
    QMenu *newSubMenu = new QMenu(QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba"), &menu);
    QAction *newSubMenuAct = menu.addAction(QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba"));
    newSubMenuAct->setMenu(newSubMenu);
    newSubMenuAct->setIcon(QIcon::fromTheme("folder-new"));

    QAction *newFolderAct = newSubMenu->addAction(QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba\xe6\x96\x87\xe4\xbb\xb6\xe5\xa4\xb9"));
    QAction *newTextFileAct = newSubMenu->addAction(QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba\xe6\x96\x87\xe6\x9c\xac\xe6\x96\x87\xe4\xbb\xb6"));
    QAction *newOfficeAct = newSubMenu->addAction(QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba Office \xe7\xb3\xbb\xe5\x88\x97\xe5\xb8\xb8\xe8\xa7\x81\xe6\x96\x87\xe4\xbb\xb6"));
    newSubMenu->addSeparator();
    QAction *newPngAct = newSubMenu->addAction(QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba PNG \xe5\x9b\xbe\xe7\x89\x87"));
    QAction *newBmpAct = newSubMenu->addAction(QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba BMP \xe5\x9b\xbe\xe7\x89\x87"));
    menu.addSeparator();

    // 立即同步
    QAction *syncAct = menu.addAction(QStringLiteral("\xe7\xab\x8b\xe5\x8d\xb3\xe5\x90\x8c\xe6\xad\xa5"));
    // 在资源管理器中打开
    QAction *openInExplorerAct = menu.addAction(QStringLiteral("\xe5\x9c\xa8\xe8\xb5\x84\xe6\xba\x90\xe7\xae\xa1\xe7\x90\x86\xe5\x99\xa8\xe4\xb8\xad\xe6\x89\x93\xe5\xbc\x80"));

    // Disable items that require selection
    if (selectedPath.isEmpty()) {
        copyAct->setEnabled(false);
        pasteAct->setEnabled(false);
        deleteAct->setEnabled(false);
        renameAct->setEnabled(false);
        compressAct->setEnabled(false);
    } else {
        if (!isDir) {
            pasteAct->setEnabled(false);
        }
    }

    QAction *triggered = menu.exec(m_fileTableView->viewport()->mapToGlobal(pos));
    if (!triggered) return;

    // Helper: target dir for new file creation
    auto targetDir = [&]() -> QString {
        if (selectedPath.isEmpty()) return m_currentPath;
        return isDir ? selectedPath : QFileInfo(selectedPath).dir().absolutePath();
    };

    if (triggered == openAct) {
        if (selectedPath.isEmpty()) return;
        if (isDir) {
            navigateTo(selectedPath);
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(selectedPath));
        }
    } else if (triggered == refreshAct) {
        onRefreshClicked();
    } else if (triggered == copyAct) {
        if (!selectedPath.isEmpty()) {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(selectedPath);
        }
    } else if (triggered == pasteAct) {
        QString tgt = targetDir();
        QClipboard *clipboard = QApplication::clipboard();
        const QMimeData *mime = clipboard->mimeData();
        QStringList sourcePaths;
        if (mime->hasUrls()) {
            for (const QUrl &url : mime->urls()) {
                QString path = url.toLocalFile();
                if (!path.isEmpty()) sourcePaths.append(path);
            }
        }
        if (sourcePaths.isEmpty()) return;
        int copied = 0;
        for (const QString &srcPath : sourcePaths) {
            QString destPath = tgt + "/" + QFileInfo(srcPath).fileName();
            if (QFileInfo(srcPath).isDir()) {
                copyDirectory(srcPath, destPath);
                copied++;
            } else {
                if (QFile::copy(srcPath, destPath)) copied++;
            }
        }
        if (copied > 0) m_fileModel->load(m_currentPath);
    } else if (triggered == deleteAct) {
        if (selectedPath.isEmpty()) return;
        int ret = QMessageBox::question(this, QStringLiteral("\xe7\xa1\xae\xe8\xae\xa4\xe5\x88\xa0\xe9\x99\xa4"),
                                        QStringLiteral("\xe7\xa1\xae\xe5\xae\x9a\xe8\xa6\x81\xe5\x88\xa0\xe9\x99\xa4 \"%1\" \xe5\x90\x97\xef\xbc\x9f").arg(selectedPath));
        if (ret == QMessageBox::Yes) {
            if (isDir) QDir(selectedPath).removeRecursively();
            else QFile::remove(selectedPath);
            m_fileModel->load(m_currentPath);
        }
    } else if (triggered == renameAct) {
        if (selectedPath.isEmpty()) return;
        bool ok = false;
        QString newName = QInputDialog::getText(this, QStringLiteral("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d"),
                                               QStringLiteral("\xe6\x96\xb0\xe5\x90\x8d\xe7\xa7\xb0:"),
                                               QLineEdit::Normal, QFileInfo(selectedPath).fileName(), &ok);
        if (ok && !newName.isEmpty() && newName != QFileInfo(selectedPath).fileName()) {
            QString newPath = QFileInfo(selectedPath).dir().absolutePath() + "/" + newName;
            QFile::rename(selectedPath, newPath);
            m_fileModel->load(m_currentPath);
        }
    } else if (triggered == compressAct) {
        if (selectedPath.isEmpty()) return;
        QString zipPath = isDir
            ? QFileInfo(selectedPath).dir().absolutePath() + "/" + QFileInfo(selectedPath).fileName() + ".zip"
            : selectedPath + ".zip";
        if (QFile::exists(zipPath)) {
            int ret = QMessageBox::question(this, QStringLiteral("\xe5\x8e\x8b\xe7\xbc\xa9"),
                    QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba\xe6\x96\x87\xe4\xbb\xb6\xe5\xb7\xb2\xe5\xad\x98\xe5\x9c\xa8\xff0c\xe8\xa6\x81\xe8\xa6\x86\xe5\x80\x92\xe5\x90\x8c\xe5\x90\x97\xef\xbc\x9f"));
            if (ret != QMessageBox::Yes) return;
        }
        compressToZip(selectedPath, zipPath, isDir);
    } else if (triggered == newFolderAct) {
        bool ok = false;
        QString name = QInputDialog::getText(this, QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba\xe6\x96\x87\xe4\xbb\xb6\xe5\xa4\xb9"),
                                            QStringLiteral("\xe6\x96\x87\xe4\xbb\xb6\xe5\xa4\xb9\xe5\x90\x8d\xe7\xa7\xb0:"),
                                            QLineEdit::Normal, QString(), &ok);
        if (ok && !name.isEmpty()) {
            QDir().mkpath(targetDir() + "/" + name);
            m_fileModel->load(m_currentPath);
        }
    } else if (triggered == newTextFileAct) {
        createNewFile(targetDir(), "txt");
    } else if (triggered == newOfficeAct) {
        createNewOfficeFile(targetDir());
    } else if (triggered == newPngAct) {
        createNewImage(targetDir(), "png");
    } else if (triggered == newBmpAct) {
        createNewImage(targetDir(), "bmp");
    } else if (triggered == syncAct) {
        onManualSync();
    } else if (triggered == openInExplorerAct) {
        QString pathToOpen = selectedPath.isEmpty() ? m_currentPath : selectedPath;
        QDesktopServices::openUrl(QUrl::fromLocalFile(pathToOpen));
    }
}


static QString suggestNewName(const QString &dir, const QString &base, const QString &ext) {
    if (ext.isEmpty()) {
        QString path = dir + "/" + base;
        if (!QFile::exists(path)) return base;
    } else {
        QString path = dir + "/" + base + "." + ext;
        if (!QFile::exists(path)) return base + "." + ext;
    }
    for (int i = 2; i < 1000; i++) {
        QString candidate = ext.isEmpty()
            ? QStringLiteral("%1 (%2)").arg(base).arg(i)
            : QStringLiteral("%1 (%2).%3").arg(base).arg(i).arg(ext);
        QString path = dir + "/" + candidate;
        if (!QFile::exists(path)) return candidate;
    }
    return QString();
}

void MainWindow::createNewFile(const QString &dir, const QString &ext)
{
    QString baseName = ext == "txt" ? QStringLiteral("ææ¬æä»¶")
                 : QStringLiteral("newfile");
    QString name = suggestNewName(dir, baseName, ext);
    QString path = dir + "/" + name;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.close();
        m_fileModel->load(m_currentPath);
    }
}


void MainWindow::createNewOfficeFile(const QString &dir)
{
    // Let user choose type: docx, xlsx, pptx
    QStringList types = {QStringLiteral("Word (\xe6\x96\x87\xe6\x9c\xac)"), QStringLiteral("Excel (\xe8\xa1\xa8\xe6\xa0\xbc)"), QStringLiteral("PowerPoint (\xe6\x8f\x92\xe5\xb9\x95)")};
    QStringList exts = {"docx", "xlsx", "pptx"};
    bool ok = false;
    QString chosen = QInputDialog::getItem(this, QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba Office \xe6\x96\x87\xe4\xbb\xb6"),
                                           QStringLiteral("\xe9\x80\x89\xe6\x8b\xa9\xe7\xb1\xbb\xe5\x9e\x8b:"), types, 0, false, &ok);
    if (!ok) return;
    int idx = types.indexOf(chosen);
    QString ext = exts.value(idx, "docx");
    QString baseName = ext == "docx" ? QStringLiteral("\xe6\x96\x87\xe6\x9c\xac")
                   : ext == "xlsx" ? QStringLiteral("\xe8\xa1\xa8\xe6\xa0\xbc")
                   : QStringLiteral("\xe6\x8f\x92\xe5\xb9\x95");
    QString name = suggestNewName(dir, baseName, ext);
    QString path = dir + "/" + name;

    // Create minimal placeholder - actual Office content is complex
    // Users should replace with real documents
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.close();
    }
    m_fileModel->load(m_currentPath);
}


void MainWindow::createNewImage(const QString &dir, const QString &format)
{
    // Create a minimal 1x1 pixel image using Python PIL/Pillow if available,
    // otherwise create a minimal BMP header manually
    QString baseName = format == "png" ? QStringLiteral("å¾ç") : QStringLiteral("å¾ç");
    QString name = suggestNewName(dir, baseName, format);
    QString path = dir + "/" + name;

    QProcess p;
    QStringList args;
    if (format == "png") {
        args = {"-c", "from PIL import Image; img = Image.new('RGB', (1,1)); img.save('" + path + "')"};
    } else {
        // BMP - minimal 1x1 24-bit BMP
        args = {"-c", "with open('" + path + "', 'wb') as f: f.write(b'BM\x1e\x00\x00\x00\x00\x00\x00\x1a\x00\x00\x00\x12\x00\x00\x00\x28\x00\x00\x00\x01\x00\x00\x00\x01\x00\x00\x00\x01\x00\x18\x00\x00\x00\x00\x00\x00\x1e\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')"};
    }
    p.setProgram("python3");
    p.setArguments(args);
    p.start();
    p.waitForFinished();

    if (p.exitCode() != 0 || !QFile::exists(path)) {
        // Fallback: minimal BMP
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            QByteArray minimalBmp = QByteArray::fromHex("424d1e000000000000001a000000120000002800000001000000010000000100180000000000001e000000000000000000000000000000000000000000000000");
            file.write(minimalBmp);
            file.close();
        }
    }
    m_fileModel->load(m_currentPath);
}


static bool copyDirectory(const QString &srcPath, const QString &destPath)
{
    QDir srcDir(srcPath);
    if (!srcDir.exists()) return false;
    QDir destDir(destPath);
    if (!destDir.exists()) {
        destDir.mkpath(destDir.absolutePath());
    }
    QDirIterator it(srcPath, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString item = it.next();
        QString itemName = QFileInfo(item).fileName();
        QString destItem = destDir.absolutePath() + "/" + itemName;
        if (QFileInfo(item).isDir()) {
            destDir.mkpath(destItem);
            copyDirectory(item, destItem);
        } else {
            QFile::copy(item, destItem);
        }
    }
    return true;
}


void MainWindow::compressToZip(const QString &sourcePath, const QString &zipPath, bool isDir)
{
    int error = 0;
    zip_t *zip = zip_open(zipPath.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!zip) {
        QMessageBox::warning(this, QStringLiteral("Compression failed"),
                            QStringLiteral("Failed to create zip file (error %1)").arg(error));
        return;
    }

    if (isDir) {
        QDirIterator it(sourcePath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QString relPath = QFileInfo(filePath).fileName();
            // Compute relative path from source parent dir
            QDir srcDir(sourcePath);
            QString arcPath = srcDir.relativeFilePath(filePath);
            zip_error_t zerr;
            zip_error_init(&zerr);
            zip_source_t *src = zip_source_file_create(filePath.toUtf8().constData(), 0, -1, &zerr);
            if (src) {
                zip_file_add(zip, arcPath.toUtf8().constData(), src, ZIP_FL_ENC_UTF_8);
            } else {
                zip_error_fini(&zerr);
                break;
            }
        }
    } else {
        zip_source_t *src = zip_source_file_create(sourcePath.toUtf8().constData(), 0, -1, nullptr);
        if (src) {
            zip_file_add(zip, QFileInfo(sourcePath).fileName().toUtf8().constData(), src, ZIP_FL_ENC_UTF_8);
        }
    }

    zip_close(zip);

    if (QFile::exists(zipPath)) {
        QMessageBox::information(this, QStringLiteral("Compressed"),
                                QStringLiteral("Compressed to: %1").arg(zipPath));
    } else {
        QMessageBox::warning(this, QStringLiteral("Compression failed"),
                            QStringLiteral("Failed to create zip file"));
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

void MainWindow::onRepositoryFocused(const QString &path)
{
    if (path.isEmpty()) return;
    m_currentPath = path;
    m_pathEdit->setText(path);
    m_statusPathLabel->setText(path);
    m_fileModel->load(path);
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
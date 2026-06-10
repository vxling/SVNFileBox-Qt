#include "copyprogressdialog.h"
#include "../sync/syncrecordservice.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QHeaderView>
#include <QCoreApplication>
#include <QDirIterator>
#include <QDir>

static bool copyDirectoryRecursive(const QString &src, const QString &dest, QThread *thr);

static QString formatBytesStatic(qint64 bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " GB";
}

// ── PasteTask ─────────────────────────────────────────────────
PasteTask::PasteTask(const QList<Item> &items, QObject *parent)
    : QThread(parent), m_items(items)
{
}

void PasteTask::run()
{
    emit stageChanged(0);  // analyzing stage

    // Stage 1: count total files and size
    int totalFiles = 0;
    qint64 totalBytes = 0;
    for (const Item &item : m_items) {
        if (item.isDir) {
            // Walk directory
            QDirIterator it(item.srcPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                totalFiles++;
                if (!it.fileInfo().isDir())
                    totalBytes += it.fileInfo().size();
                emit progressChanged(totalFiles, totalFiles, totalBytes, totalBytes, it.filePath(), QStringLiteral("正在分析..."));
            }
        } else {
            totalFiles++;
            totalBytes += item.size;
            emit progressChanged(totalFiles, totalFiles, totalBytes, totalBytes, item.srcPath, QStringLiteral("正在分析..."));
        }
    }

    // Stage 2: copy
    emit stageChanged(1);  // copying stage
    int current = 0;
    int copied = 0;
    int failed = 0;
    qint64 bytesDone = 0;
    for (const Item &item : m_items) {
        current++;
        if (item.isDir) {
            copyDirectoryRecursive(item.srcPath, item.destPath, this);
            copied++;
        } else {
            if (QFile::copy(item.srcPath, item.destPath)) {
                copied++;
                bytesDone += item.size;
            } else {
                failed++;
            }
            emit progressChanged(current, totalFiles, bytesDone, totalBytes, item.srcPath, QStringLiteral("正在复制..."));
        }
    }
    emit copyFinished(copied, failed);
}

static bool copyDirectoryRecursive(const QString &src, const QString &dest, QThread *thr)
{
    QDir srcDir(src);
    if (!srcDir.exists()) return false;
    QDir destDir(dest);
    if (!destDir.exists()) destDir.mkpath(dest);

    QDirIterator it(src, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (thr->isInterruptionRequested()) return false;
        QString item = it.next();
        QString name = QFileInfo(item).fileName();
        QString destItem = dest + "/" + name;
        if (QFileInfo(item).isDir()) {
            destDir.mkpath(destItem);
            copyDirectoryRecursive(item, destItem, thr);
        } else {
            QFile::copy(item, destItem);
        }
    }
    return true;
}

// ── CopyProgressDialog ─────────────────────────────────────────
CopyProgressDialog::CopyProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("正在粘贴"));
    setFixedSize(520, 200);
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(10);

    // Stage label (analyzing / copying)
    m_stageLabel = new QLabel(QStringLiteral("正在分析..."));
    m_stageLabel->setStyleSheet(QStringLiteral("QLabel { font-size: 13px; font-weight: bold; color: #1E88E5; }"));
    layout->addWidget(m_stageLabel);

    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setFixedHeight(10);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%p%");
    m_progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar { border: 1px solid #E0E0E0; border-radius: 5px; background: #F0F0F0; min-height: 10px; }"
        "QProgressBar::chunk { background: #1E88E5; border-radius: 5px; }"));
    layout->addWidget(m_progressBar);

    // File count
    m_fileCountLabel = new QLabel();
    m_fileCountLabel->setStyleSheet(QStringLiteral("QLabel { font-size: 12px; color: #555; }"));
    layout->addWidget(m_fileCountLabel);

    // Bytes progress
    m_bytesLabel = new QLabel();
    m_bytesLabel->setStyleSheet(QStringLiteral("QLabel { font-size: 12px; color: #888; }"));
    layout->addWidget(m_bytesLabel);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelBtn = new QPushButton(QStringLiteral("取消"));
    m_cancelBtn->setFixedSize(90, 34);
    btnLayout->addWidget(m_cancelBtn);

    m_closeBtn = new QPushButton(QStringLiteral("关闭"));
    m_closeBtn->setFixedSize(90, 34);
    m_closeBtn->setVisible(false);
    m_closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"));
    btnLayout->addWidget(m_closeBtn);

    layout->addLayout(btnLayout);

    connect(m_cancelBtn, &QPushButton::clicked, this, &CopyProgressDialog::onCancelClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

CopyProgressDialog::~CopyProgressDialog() = default;

void CopyProgressDialog::setStage(int stage)
{
    m_stageLabel->setText(stage == 0 ? QStringLiteral("正在分析...") : QStringLiteral("正在复制..."));
    m_stageLabel->setStyleSheet(stage == 0
        ? QStringLiteral("QLabel { font-size: 13px; font-weight: bold; color: #FFA000; }")
        : QStringLiteral("QLabel { font-size: 13px; font-weight: bold; color: #1E88E5; }"));
}

void CopyProgressDialog::setProgress(int current, int total, qint64 bytesDone,
                                     qint64 totalBytes, const QString &currentFile,
                                     const QString &stageLabel)
{
    if (total > 0) {
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(current);
    }
    m_stageLabel->setText(stageLabel);
    m_fileCountLabel->setText(QStringLiteral("已处理: %1 / %2 个文件").arg(current).arg(total));
    if (totalBytes > 0) {
        m_bytesLabel->setText(QStringLiteral("%1 / %2").arg(formatBytesStatic(bytesDone)).arg(formatBytesStatic(totalBytes)));
    } else {
        m_bytesLabel->setText(QString());
    }
}

void CopyProgressDialog::reset()
{
    m_cancelled = false;
    m_done = false;
    m_progressBar->setValue(0);
    m_stageLabel->setText(QStringLiteral("正在分析..."));
    m_stageLabel->setStyleSheet(QStringLiteral("QLabel { font-size: 13px; font-weight: bold; color: #FFA000; }"));
    m_fileCountLabel->clear();
    m_bytesLabel->clear();
    m_cancelBtn->setVisible(true);
    m_closeBtn->setVisible(false);
}

void CopyProgressDialog::reject()
{
    if (m_done) {
        QDialog::reject();
    } else {
        onCancelClicked();
    }
}

void CopyProgressDialog::onCancelClicked()
{
    m_cancelled = true;
    emit cancelled();
    reject();
}

void CopyProgressDialog::markDone(const QString &message)
{
    m_done = true;
    m_stageLabel->setText(message);
    m_progressBar->setValue(m_progressBar->maximum());
    m_cancelBtn->setVisible(false);
    m_closeBtn->setVisible(true);
}

void CopyProgressDialog::showEvent(QShowEvent *event)
{
    reset();
    QDialog::showEvent(event);
}

// ── SyncRecordsDialog ───────────────────────────────────────────
SyncRecordsDialog::SyncRecordsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("同步记录"));
    setFixedSize(700, 450);
    setModal(false);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 12);
    layout->setSpacing(12);

    QLabel *hint = new QLabel(QStringLiteral("以下是所有仓库的同步历史记录："));
    hint->setStyleSheet(QStringLiteral("QLabel { font-size: 12px; color: #666; }"));
    layout->addWidget(hint);

    m_tableView = new QTableView();
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setVisible(false);

    SyncRecordService *recordService = SyncRecordService::instance();
    m_tableView->setModel(recordService);

    m_tableView->setColumnWidth(0, 200);
    m_tableView->setColumnWidth(1, 300);
    m_tableView->setColumnWidth(2, 100);

    layout->addWidget(m_tableView, 1);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(QStringLiteral("关闭"));
    closeBtn->setFixedSize(100, 36);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"));

    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

SyncRecordsDialog::~SyncRecordsDialog() = default;

// ── AboutDialog ────────────────────────────────────────────────
AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("关于 SVNFileBox"));
    setFixedSize(400, 280);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 16);
    layout->setSpacing(16);

    QLabel *title = new QLabel(QStringLiteral("SVNFileBox"));
    title->setStyleSheet(QStringLiteral("QLabel { font-size: 20px; font-weight: bold; color: #1A1A2E; }"));
    layout->addWidget(title);

    layout->addWidget(new QLabel(QStringLiteral("版本: 1.0.0")));
    layout->addWidget(new QLabel(QStringLiteral("跨平台 SVN 同步客户端")));
    layout->addWidget(new QLabel(QStringLiteral("基于 Qt 6 + Qt Widgets 构建")));

    layout->addStretch();

    QLabel *copyright = new QLabel(QStringLiteral("© 2024 vxling"));
    copyright->setStyleSheet(QStringLiteral("QLabel { font-size: 11px; color: #999; }"));
    layout->addWidget(copyright);

    QPushButton *closeBtn = new QPushButton(QStringLiteral("关闭"));
    closeBtn->setFixedSize(100, 36);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"));

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

AboutDialog::~AboutDialog() = default;

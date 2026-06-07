#include "copyprogressdialog.h"
#include "../sync/syncrecordservice.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QHeaderView>

static QString formatBytesStatic(qint64 bytes)
{
    if (bytes < 1024) return QString::number(bytes) + QStringLiteral(" B");
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + QStringLiteral(" KB");
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024), 'f', 1) + QStringLiteral(" MB");
    return QString::number(bytes / (1024.0 * 1024 * 1024), 'f', 1) + QStringLiteral(" GB");
}

// ── CopyProgressDialog ──────────────────────────────────────────
CopyProgressDialog::CopyProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("导入文件"));
    setFixedSize(480, 160);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 16);
    layout->setSpacing(12);

    m_fileLabel = new QLabel();
    m_fileLabel->setStyleSheet(QStringLiteral("QLabel { font-size: 13px; color: #333; }"));
    m_fileLabel->setWordWrap(true);
    layout->addWidget(m_fileLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar { border: 1px solid #E0E0E0; border-radius: 4px; background: #F5F5F5; }"
        "QProgressBar::chunk { background: #1E88E5; border-radius: 4px; }"));
    layout->addWidget(m_progressBar);

    m_bytesLabel = new QLabel();
    m_bytesLabel->setStyleSheet(QStringLiteral("QLabel { font-size: 12px; color: #666; }"));
    layout->addWidget(m_bytesLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelBtn = new QPushButton(QStringLiteral("取消"));
    m_cancelBtn->setFixedSize(100, 36);
    btnLayout->addWidget(m_cancelBtn);
    layout->addLayout(btnLayout);

    connect(m_cancelBtn, &QPushButton::clicked, this, &CopyProgressDialog::onCancelClicked);
}

CopyProgressDialog::~CopyProgressDialog() = default;

void CopyProgressDialog::setProgress(int current, int total, qint64 bytesCopied,
                                     qint64 totalBytes, const QString &currentFile)
{
    if (total > 0) {
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(current);
    }
    m_fileLabel->setText(QStringLiteral("正在复制: %1").arg(currentFile));
    m_bytesLabel->setText(QStringLiteral("%1 / %2")
        .arg(formatBytesStatic(bytesCopied))
        .arg(formatBytesStatic(totalBytes)));
}

void CopyProgressDialog::reset()
{
    m_wasCancelled = false;
    m_progressBar->setValue(0);
    m_fileLabel->clear();
    m_bytesLabel->clear();
}

void CopyProgressDialog::onCancelClicked()
{
    m_wasCancelled = true;
    emit cancelled();
    reject();
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

    m_tableView->setColumnWidth(0, 200); // repo name
    m_tableView->setColumnWidth(1, 300); // file path
    m_tableView->setColumnWidth(2, 100); // action

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
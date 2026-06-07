#include "newfiledialog.h"
#include "../models/filemodel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QFileInfo>
#include <QInputDialog>

// ── NewFileDialog ──────────────────────────────────────────────
NewFileDialog::NewFileDialog(FileModel *fileModel, const QString &currentPath, QWidget *parent)
    : QDialog(parent)
    , m_fileModel(fileModel)
    , m_currentPath(currentPath)
    , m_ext(QStringLiteral("txt"))
{
    setWindowTitle(QStringLiteral("新建文件"));
    setFixedSize(360, 180);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    QLabel *title = new QLabel(QStringLiteral("新建文件"));
    title->setStyleSheet(QStringLiteral("QLabel { font-size: 16px; font-weight: bold; }"));
    layout->addWidget(title);

    m_nameInput = new QLineEdit();
    m_nameInput->setFixedHeight(36);
    m_nameInput->setPlaceholderText(QStringLiteral("新建文件"));
    layout->addWidget(m_nameInput);

    m_typeLabel = new QLabel(QStringLiteral("类型: .") + m_ext);
    m_typeLabel->setStyleSheet(QStringLiteral("QLabel { color: #666; font-size: 12px; }"));
    layout->addWidget(m_typeLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *confirmBtn = new QPushButton(QStringLiteral("确定"));
    confirmBtn->setFixedSize(100, 36);
    confirmBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"));

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setFixedSize(80, 36);

    btnLayout->addWidget(confirmBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(confirmBtn, &QPushButton::clicked, this, &NewFileDialog::onConfirmClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &NewFileDialog::onCancelClicked);
}

NewFileDialog::~NewFileDialog() = default;

void NewFileDialog::onConfirmClicked()
{
    QString name = m_nameInput->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入文件名"));
        return;
    }
    if (!name.endsWith(QStringLiteral(".") + m_ext))
        name += QStringLiteral(".") + m_ext;
    QString fullPath = m_currentPath + QStringLiteral("/") + name;
    if (m_fileModel->createFile(fullPath)) {
        accept();
    } else {
        QMessageBox::warning(this, QStringLiteral("错误"),
            QStringLiteral("创建文件失败: %1").arg(fullPath));
    }
}

void NewFileDialog::onCancelClicked()
{
    reject();
}

// ── NewFolderDialog ─────────────────────────────────────────────
NewFolderDialog::NewFolderDialog(FileModel *fileModel, const QString &currentPath, QWidget *parent)
    : QDialog(parent)
    , m_fileModel(fileModel)
    , m_currentPath(currentPath)
{
    setWindowTitle(QStringLiteral("新建文件夹"));
    setFixedSize(360, 160);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    QLabel *title = new QLabel(QStringLiteral("新建文件夹"));
    title->setStyleSheet(QStringLiteral("QLabel { font-size: 16px; font-weight: bold; }"));
    layout->addWidget(title);

    m_nameInput = new QLineEdit();
    m_nameInput->setFixedHeight(36);
    m_nameInput->setPlaceholderText(QStringLiteral("新文件夹"));
    layout->addWidget(m_nameInput);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *confirmBtn = new QPushButton(QStringLiteral("确定"));
    confirmBtn->setFixedSize(100, 36);
    confirmBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"));

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setFixedSize(80, 36);

    btnLayout->addWidget(confirmBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(confirmBtn, &QPushButton::clicked, this, &NewFolderDialog::onConfirmClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &NewFolderDialog::onCancelClicked);
}

NewFolderDialog::~NewFolderDialog() = default;

void NewFolderDialog::onConfirmClicked()
{
    QString name = m_nameInput->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入文件夹名称"));
        return;
    }
    QString fullPath = m_currentPath + QStringLiteral("/") + name;
    if (m_fileModel->createDirectory(fullPath)) {
        accept();
    } else {
        QMessageBox::warning(this, QStringLiteral("错误"),
            QStringLiteral("创建文件夹失败: %1").arg(fullPath));
    }
}

void NewFolderDialog::onCancelClicked()
{
    reject();
}

// ── RenameDialog ───────────────────────────────────────────────
RenameDialog::RenameDialog(const QString &oldPath, const QString &oldName, QWidget *parent)
    : QDialog(parent)
    , m_oldPath(oldPath)
{
    setWindowTitle(QStringLiteral("重命名"));
    setFixedSize(360, 160);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    QLabel *title = new QLabel(QStringLiteral("重命名"));
    title->setStyleSheet(QStringLiteral("QLabel { font-size: 16px; font-weight: bold; }"));
    layout->addWidget(title);

    m_nameInput = new QLineEdit();
    m_nameInput->setFixedHeight(36);
    m_nameInput->setText(oldName);
    layout->addWidget(m_nameInput);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *confirmBtn = new QPushButton(QStringLiteral("确定"));
    confirmBtn->setFixedSize(100, 36);
    confirmBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"));

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setFixedSize(80, 36);

    btnLayout->addWidget(confirmBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(confirmBtn, &QPushButton::clicked, this, &RenameDialog::onConfirmClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &RenameDialog::onCancelClicked);
}

QString RenameDialog::newName() const
{
    return m_nameInput->text().trimmed();
}

void RenameDialog::onConfirmClicked()
{
    accept();
}

void RenameDialog::onCancelClicked()
{
    reject();
}

// ── RenameRepoDialog ────────────────────────────────────────────
RenameRepoDialog::RenameRepoDialog(const QString &oldName, QWidget *parent)
    : QDialog(parent)
    , m_oldName(oldName)
{
    setWindowTitle(QStringLiteral("重命名仓库"));
    setFixedSize(380, 180);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    QLabel *title = new QLabel(QStringLiteral("重命名仓库"));
    title->setStyleSheet(QStringLiteral("QLabel { font-size: 16px; font-weight: bold; }"));
    layout->addWidget(title);

    layout->addWidget(new QLabel(QStringLiteral("原名称：") + oldName));

    m_nameInput = new QLineEdit();
    m_nameInput->setFixedHeight(36);
    m_nameInput->setText(oldName);
    layout->addWidget(m_nameInput);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *confirmBtn = new QPushButton(QStringLiteral("确定"));
    confirmBtn->setFixedSize(100, 36);
    confirmBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"));

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setFixedSize(80, 36);

    btnLayout->addWidget(confirmBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(confirmBtn, &QPushButton::clicked, this, &RenameRepoDialog::onConfirmClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &RenameRepoDialog::onCancelClicked);
}

QString RenameRepoDialog::newName() const
{
    return m_nameInput->text().trimmed();
}

void RenameRepoDialog::onConfirmClicked()
{
    if (m_nameInput->text().trimmed().isEmpty() ||
        m_nameInput->text().trimmed() == m_oldName) {
        reject();
        return;
    }
    accept();
}

void RenameRepoDialog::onCancelClicked()
{
    reject();
}

// ── EditRepoDialog ─────────────────────────────────────────────
EditRepoDialog::EditRepoDialog(const QString &name, const QString &url, QWidget *parent)
    : QDialog(parent)
    , m_name(name)
{
    setWindowTitle(QStringLiteral("修改仓库 URL"));
    setFixedSize(420, 200);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    QLabel *title = new QLabel(QStringLiteral("修改仓库 URL"));
    title->setStyleSheet(QStringLiteral("QLabel { font-size: 16px; font-weight: bold; }"));
    layout->addWidget(title);

    layout->addWidget(new QLabel(QStringLiteral("仓库：") + name));

    m_urlInput = new QLineEdit();
    m_urlInput->setFixedHeight(36);
    m_urlInput->setText(url);
    m_urlInput->setPlaceholderText(QStringLiteral("新 URL (例如 https://svn.example.com/repo)"));
    layout->addWidget(m_urlInput);

    QLabel *note = new QLabel(QStringLiteral("注：仅修改本地记录的 URL；如需重新定位工作副本，请使用 svn switch --relocate。"));
    note->setStyleSheet(QStringLiteral("QLabel { font-size: 10px; color: #999; }"));
    note->setWordWrap(true);
    layout->addWidget(note);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: #E53935; font-size: 11px; }"));
    layout->addWidget(m_statusLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *saveBtn = new QPushButton(QStringLiteral("保存"));
    saveBtn->setFixedSize(100, 36);
    saveBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"));

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setFixedSize(80, 36);

    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(saveBtn, &QPushButton::clicked, this, &EditRepoDialog::onConfirmClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &EditRepoDialog::onCancelClicked);
}

QString EditRepoDialog::newUrl() const
{
    return m_urlInput->text().trimmed();
}

void EditRepoDialog::onConfirmClicked()
{
    if (m_urlInput->text().trimmed().isEmpty()) {
        m_statusLabel->setText(QStringLiteral("URL 不能为空"));
        return;
    }
    accept();
}

void EditRepoDialog::onCancelClicked()
{
    reject();
}

// ── ConfirmDeleteDialog ────────────────────────────────────────
ConfirmDeleteDialog::ConfirmDeleteDialog(const QString &itemName, QWidget *parent)
    : QDialog(parent)
    , m_itemName(itemName)
{
    setWindowTitle(QStringLiteral("确认删除"));
    setFixedSize(360, 160);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    QLabel *title = new QLabel(QStringLiteral("确认删除"));
    title->setStyleSheet(QStringLiteral("QLabel { font-size: 16px; font-weight: bold; }"));
    layout->addWidget(title);

    layout->addWidget(new QLabel(QStringLiteral("确定要删除「%1」吗？").arg(itemName)));

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *confirmBtn = new QPushButton(QStringLiteral("删除"));
    confirmBtn->setFixedSize(100, 36);
    confirmBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #E53935; color: white; border: none; border-radius: 4px; }"));

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setFixedSize(80, 36);

    btnLayout->addWidget(confirmBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(confirmBtn, &QPushButton::clicked, this, &ConfirmDeleteDialog::onConfirmClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &ConfirmDeleteDialog::onCancelClicked);
}

void ConfirmDeleteDialog::onConfirmClicked()
{
    accept();
}

void ConfirmDeleteDialog::onCancelClicked()
{
    reject();
}

// ── ConflictDialog ─────────────────────────────────────────────
ConflictDialog::ConflictDialog(const QStringList &files, QWidget *parent)
    : QDialog(parent)
    , m_files(files)
{
    setWindowTitle(QStringLiteral("文件冲突"));
    setFixedSize(400, 320);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    QLabel *title = new QLabel(QStringLiteral("文件冲突"));
    title->setStyleSheet(QStringLiteral("QLabel { font-size: 16px; font-weight: bold; }"));
    layout->addWidget(title);

    layout->addWidget(new QLabel(QStringLiteral("检测到以下文件存在冲突:")));
    layout->addWidget(new QLabel(files.join(QStringLiteral("\n"))));

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *resolveBtn = new QPushButton(QStringLiteral("解决"));
    resolveBtn->setFixedSize(100, 36);
    resolveBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"));

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setFixedSize(80, 36);

    btnLayout->addWidget(resolveBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(resolveBtn, &QPushButton::clicked, this, &ConflictDialog::onResolvedClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &ConflictDialog::onCancelClicked);
}

void ConflictDialog::onResolvedClicked()
{
    accept();
}

void ConflictDialog::onCancelClicked()
{
    reject();
}
#include "checkoutdialog.h"
#include "../services/repoglobalmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>

CheckoutDialog::CheckoutDialog(SVNFileBox::RepoGlobalManager *globalManager, QWidget *parent)
    : QDialog(parent)
    , m_globalManager(globalManager)
{
    setWindowTitle(QStringLiteral("从网络添加仓库"));
    setFixedSize(540, 380);
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 16);
    mainLayout->setSpacing(16);

    QFormLayout *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(12);

    m_nameInput = new QLineEdit();
    m_nameInput->setPlaceholderText(QStringLiteral("例如：我的项目"));
    m_nameInput->setFixedHeight(36);
    form->addRow(QStringLiteral("仓库名称:"), m_nameInput);

    m_urlInput = new QLineEdit();
    m_urlInput->setPlaceholderText(QStringLiteral("https://example.com/svn/repo"));
    m_urlInput->setFixedHeight(36);
    form->addRow(QStringLiteral("SVN 仓库 URL:"), m_urlInput);

    m_userInput = new QLineEdit();
    m_userInput->setPlaceholderText(QStringLiteral("（可选）"));
    m_userInput->setFixedHeight(36);
    form->addRow(QStringLiteral("用户名:"), m_userInput);

    m_passInput = new QLineEdit();
    m_passInput->setPlaceholderText(QStringLiteral("（可选）"));
    m_passInput->setFixedHeight(36);
    m_passInput->setEchoMode(QLineEdit::Password);
    form->addRow(QStringLiteral("密码:"), m_passInput);

    m_folderInput = new QLineEdit();
    m_folderInput->setPlaceholderText(QStringLiteral("选择检出目录"));
    m_folderInput->setFixedHeight(36);
    form->addRow(QStringLiteral("检出目录:"), m_folderInput);

    QHBoxLayout *folderLayout = new QHBoxLayout();
    m_browseBtn = new QPushButton(QStringLiteral("浏览..."));
    m_browseBtn->setFixedSize(90, 36);
    folderLayout->addWidget(m_folderInput, 1);
    folderLayout->addWidget(m_browseBtn);
    form->addRow(QStringLiteral(""), folderLayout);

    mainLayout->addLayout(form);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: #E53935; font-size: 12px; }"));
    mainLayout->addWidget(m_statusLabel);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *confirmBtn = new QPushButton(QStringLiteral("确认"));
    confirmBtn->setFixedSize(100, 36);
    confirmBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #1565C0; }"));

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setFixedSize(80, 36);

    btnLayout->addWidget(confirmBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_browseBtn, &QPushButton::clicked, this, &CheckoutDialog::onBrowseFolder);
    connect(confirmBtn, &QPushButton::clicked, this, &CheckoutDialog::onConfirmClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &CheckoutDialog::onCancelClicked);
}

CheckoutDialog::~CheckoutDialog() = default;

void CheckoutDialog::onBrowseFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("选择检出目录"), QDir::homePath());
    if (!dir.isEmpty()) {
        m_folderInput->setText(dir);
    }
}

void CheckoutDialog::onConfirmClicked()
{
    QString name = m_nameInput->text().trimmed();
    QString url = m_urlInput->text().trimmed();
    QString folder = m_folderInput->text().trimmed();
    QString user = m_userInput->text().trimmed();
    QString pass = m_passInput->text();

    if (name.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("仓库名称不能为空"));
        return;
    }
    if (url.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("SVN 仓库 URL 不能为空"));
        return;
    }
    if (folder.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("检出目录不能为空"));
        return;
    }

    m_globalManager->createNetworkRepoAsync(name, folder, url, user, pass);
    emit checkoutCompleted(name, folder);
    accept();
}

void CheckoutDialog::onCancelClicked()
{
    reject();
}
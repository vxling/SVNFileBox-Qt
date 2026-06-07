#include "checkoutdialog.h"
#include "ui_checkoutdialog.h"
#include "../services/repoglobalmanager.h"

#include <QFileDialog>
#include <QMessageBox>

CheckoutDialog::CheckoutDialog(SVNFileBox::RepoGlobalManager *globalManager, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CheckoutDialog)
    , m_globalManager(globalManager)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("从网络添加仓库"));
    setFixedSize(540, 400);

    ui->confirmBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #07C160; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #06AD56; }"));
    ui->cancelBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #F5F5F5; color: #333; border: 1px solid #E0E0E0; border-radius: 4px; }"
        "QPushButton:hover { background: #E0E0E0; }"));

    connect(ui->browseBtn, &QPushButton::clicked, this, &CheckoutDialog::onBrowseFolder);
    connect(ui->confirmBtn, &QPushButton::clicked, this, &CheckoutDialog::onConfirmClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &CheckoutDialog::onCancelClicked);
}

CheckoutDialog::~CheckoutDialog() = default;

void CheckoutDialog::onBrowseFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("选择检出目录"), QDir::homePath());
    if (!dir.isEmpty()) {
        ui->folderInput->setText(dir);
    }
}

void CheckoutDialog::onConfirmClicked()
{
    QString name = ui->nameInput->text().trimmed();
    QString url = ui->urlInput->text().trimmed();
    QString folder = ui->folderInput->text().trimmed();
    QString user = ui->userInput->text().trimmed();
    QString pass = ui->passInput->text();

    if (name.isEmpty()) {
        ui->statusLabel->setText(QStringLiteral("仓库名称不能为空"));
        return;
    }
    if (url.isEmpty()) {
        ui->statusLabel->setText(QStringLiteral("SVN 仓库 URL 不能为空"));
        return;
    }
    if (folder.isEmpty()) {
        ui->statusLabel->setText(QStringLiteral("检出目录不能为空"));
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
#include "addlocaldialog.h"
#include "ui_addlocaldialog.h"
#include "../services/repoglobalmanager.h"
#include "../config/configservice.h"

#include <QFileInfo>
#include <QFileDialog>

AddLocalDialog::AddLocalDialog(SVNFileBox::RepoGlobalManager *globalManager,
                               ConfigService *configService, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddLocalDialog)
    , m_globalManager(globalManager)
    , m_configService(configService)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("添加本地仓库"));
    setFixedSize(500, 200);

    ui->confirmBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #07C160; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #06AD56; }"));
    ui->cancelBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #F5F5F5; color: #333; border: 1px solid #E0E0E0; border-radius: 4px; }"
        "QPushButton:hover { background: #E0E0E0; }"));

    connect(ui->browseBtn, &QPushButton::clicked, this, &AddLocalDialog::onBrowseClicked);
    connect(ui->confirmBtn, &QPushButton::clicked, this, &AddLocalDialog::onConfirmClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &AddLocalDialog::onCancelClicked);
}

AddLocalDialog::~AddLocalDialog() = default;

void AddLocalDialog::onBrowseClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("选择 SVN 工作副本目录"), QDir::homePath());
    if (!dir.isEmpty()) {
        ui->pathInput->setText(dir);
    }
}

void AddLocalDialog::onConfirmClicked()
{
    QString path = ui->pathInput->text().trimmed();
    if (path.isEmpty()) {
        ui->statusLabel->setText(QStringLiteral("请选择工作副本目录"));
        return;
    }

    QString name = QFileInfo(path).fileName();
    if (name.isEmpty()) name = QStringLiteral("未命名仓库");

    QVariantMap repoMap;
    repoMap[QStringLiteral("name")] = name;
    repoMap[QStringLiteral("path")] = path;
    repoMap[QStringLiteral("url")] = QString();
    repoMap[QStringLiteral("type")] = QStringLiteral("Local");
    repoMap[QStringLiteral("isSelected")] = true;
    if (m_configService) {
        m_configService->addRepository(repoMap);
        m_configService->saveConfig();
    }

    m_globalManager->createLocalRepo(name, path, QString(), QString(), QString());
    emit localRepoAdded(name, path);
    accept();
}

void AddLocalDialog::onCancelClicked()
{
    reject();
}
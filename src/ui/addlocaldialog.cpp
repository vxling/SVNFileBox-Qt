#include "addlocaldialog.h"
#include "../services/repoglobalmanager.h"
#include "../config/configservice.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QFileInfo>

AddLocalDialog::AddLocalDialog(SVNFileBox::RepoGlobalManager *globalManager,
                               ConfigService *configService, QWidget *parent)
    : QDialog(parent)
    , m_globalManager(globalManager)
    , m_configService(configService)
{
    setWindowTitle(QStringLiteral("添加本地仓库"));
    setFixedSize(500, 200);
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 16);
    mainLayout->setSpacing(12);

    QLabel *descLabel = new QLabel(QStringLiteral("选择一个已有的 SVN 工作副本目录"));
    descLabel->setStyleSheet(QStringLiteral("QLabel { font-size: 13px; color: #666666; }"));
    mainLayout->addWidget(descLabel);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->setSpacing(12);

    m_pathInput = new QLineEdit();
    m_pathInput->setPlaceholderText(QStringLiteral("选择本地 SVN 工作副本目录"));
    m_pathInput->setReadOnly(true);
    m_pathInput->setFixedHeight(36);
    m_pathInput->setStyleSheet(QStringLiteral(
        "QLineEdit { background: #F5F5F5; border: 1px solid #E0E0E0; border-radius: 4px; padding: 0 8px; }"));

    m_browseBtn = new QPushButton(QStringLiteral("浏览..."));
    m_browseBtn->setFixedSize(90, 36);
    pathLayout->addWidget(m_pathInput, 1);
    pathLayout->addWidget(m_browseBtn);
    mainLayout->addLayout(pathLayout);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: #E53935; font-size: 12px; }"));
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addStretch();

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

    connect(m_browseBtn, &QPushButton::clicked, this, &AddLocalDialog::onBrowseClicked);
    connect(confirmBtn, &QPushButton::clicked, this, &AddLocalDialog::onConfirmClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &AddLocalDialog::onCancelClicked);
}

AddLocalDialog::~AddLocalDialog() = default;

void AddLocalDialog::onBrowseClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("选择 SVN 工作副本目录"), QDir::homePath());
    if (!dir.isEmpty()) {
        m_pathInput->setText(dir);
    }
}

void AddLocalDialog::onConfirmClicked()
{
    QString path = m_pathInput->text().trimmed();
    if (path.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("请选择工作副本目录"));
        return;
    }

    // Extract name from path
    QString name = QFileInfo(path).fileName();
    if (name.isEmpty()) name = QStringLiteral("未命名仓库");

    // Persist to ConfigService first
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

    // Create manager in global manager (also switches to it)
    m_globalManager->createLocalRepo(name, path, QString(), QString(), QString());

    emit localRepoAdded(name, path);
    accept();
}

void AddLocalDialog::onCancelClicked()
{
    reject();
}
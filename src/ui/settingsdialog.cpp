#include "settingsdialog.h"
#include "../config/configservice.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QMessageBox>

SettingsDialog::SettingsDialog(ConfigService *configService, QWidget *parent)
    : QDialog(parent)
    , m_configService(configService)
{
    setWindowTitle(QStringLiteral("设置"));
    setFixedSize(480, 420);
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 16);
    mainLayout->setSpacing(12);

    // Auto sync
    QFormLayout *form = new QFormLayout();
    form->setSpacing(12);

    m_autoSyncCheck = new QCheckBox(QStringLiteral("启用自动同步"));
    form->addRow(QStringLiteral("自动同步:"), m_autoSyncCheck);

    m_syncIntervalSpin = new QSpinBox();
    m_syncIntervalSpin->setRange(1, 1440);
    m_syncIntervalSpin->setSuffix(QStringLiteral(" 分钟"));
    form->addRow(QStringLiteral("同步周期:"), m_syncIntervalSpin);

    m_proxyUrlInput = new QLineEdit();
    m_proxyUrlInput->setPlaceholderText(QStringLiteral("http://proxy:8080"));
    form->addRow(QStringLiteral("HTTP 代理:"), m_proxyUrlInput);

    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setRange(5, 300);
    m_timeoutSpin->setSuffix(QStringLiteral(" 秒"));
    form->addRow(QStringLiteral("文件传输超时:"), m_timeoutSpin);

    m_autoStartCheck = new QCheckBox(QStringLiteral("开机启动"));
    form->addRow(QStringLiteral("开机启动:"), m_autoStartCheck);

    m_minimizeToTrayCheck = new QCheckBox(QStringLiteral("最小化到托盘"));
    form->addRow(QStringLiteral("最小化到托盘:"), m_minimizeToTrayCheck);

    m_autoStartMinimizeCheck = new QCheckBox(QStringLiteral("开机启动时最小化到托盘"));
    form->addRow(QStringLiteral("开机启动最小化:"), m_autoStartMinimizeCheck);

    m_languageCombo = new QComboBox();
    m_languageCombo->addItems({QStringLiteral("自动检测"), QStringLiteral("简体中文"), QStringLiteral("English")});
    form->addRow(QStringLiteral("语言:"), m_languageCombo);

    m_themeCombo = new QComboBox();
    m_themeCombo->addItems({QStringLiteral("浅色"), QStringLiteral("深色")});
    form->addRow(QStringLiteral("主题:"), m_themeCombo);

    mainLayout->addLayout(form);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *saveBtn = new QPushButton(QStringLiteral("保存"));
    saveBtn->setFixedSize(100, 36);
    saveBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1E88E5; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #1565C0; }"));

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setFixedSize(80, 36);

    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancelClicked);

    loadSettings();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::loadSettings()
{
    if (!m_configService) return;

    m_autoSyncCheck->setChecked(m_configService->autoSyncEnabled());
    m_syncIntervalSpin->setValue(m_configService->syncIntervalMinutes());
    m_proxyUrlInput->setText(m_configService->proxyUrl());
    m_autoStartCheck->setChecked(m_configService->autoStart());
    m_minimizeToTrayCheck->setChecked(m_configService->minimizeToTray());
    m_autoStartMinimizeCheck->setChecked(m_configService->autoStartMinimize());
    m_timeoutSpin->setValue(m_configService->fileTransferTimeoutSeconds());

    QString lang = m_configService->language();
    if (lang == "zh-CN")
        m_languageCombo->setCurrentIndex(1);
    else if (lang == "en")
        m_languageCombo->setCurrentIndex(2);
    else
        m_languageCombo->setCurrentIndex(0);

    QString theme = m_configService->theme();
    m_themeCombo->setCurrentIndex(theme == "dark" ? 1 : 0);
}

void SettingsDialog::onSaveClicked()
{
    m_configService->setAutoSyncEnabled(m_autoSyncCheck->isChecked());
    m_configService->setSyncIntervalMinutes(m_syncIntervalSpin->value());
    m_configService->setProxyUrl(m_proxyUrlInput->text());
    m_configService->setAutoStart(m_autoStartCheck->isChecked());
    m_configService->setMinimizeToTray(m_minimizeToTrayCheck->isChecked());
    m_configService->setAutoStartMinimize(m_autoStartMinimizeCheck->isChecked());
    m_configService->setFileTransferTimeoutSeconds(m_timeoutSpin->value());

    int langIndex = m_languageCombo->currentIndex();
    QString lang = (langIndex == 1) ? "zh-CN" : (langIndex == 2 ? "en" : "auto");
    m_configService->setLanguage(lang);

    QString theme = (m_themeCombo->currentIndex() == 1) ? "dark" : "light";
    m_configService->setTheme(theme);

    accept();
}

void SettingsDialog::onCancelClicked()
{
    reject();
}
#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "../config/configservice.h"

#include <QMessageBox>

SettingsDialog::SettingsDialog(ConfigService *configService, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_configService(configService)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("设置"));
    setFixedSize(480, 440);

    // Style the buttons
    ui->saveBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #07C160; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #06AD56; }"));
    ui->cancelBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #F5F5F5; color: #333; border: 1px solid #E0E0E0; border-radius: 4px; }"
        "QPushButton:hover { background: #E0E0E0; }"));

    connect(ui->saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancelClicked);

    loadSettings();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::loadSettings()
{
    if (!m_configService) return;

    ui->autoSyncCheck->setChecked(m_configService->autoSyncEnabled());
    ui->syncIntervalSpin->setValue(m_configService->syncIntervalMinutes());
    ui->proxyUrlInput->setText(m_configService->proxyUrl());
    ui->autoStartCheck->setChecked(m_configService->autoStart());
    ui->minimizeToTrayCheck->setChecked(m_configService->minimizeToTray());
    ui->autoStartMinimizeCheck->setChecked(m_configService->autoStartMinimize());
    ui->timeoutSpin->setValue(m_configService->fileTransferTimeoutSeconds());

    QString lang = m_configService->language();
    if (lang == "zh-CN")
        ui->languageCombo->setCurrentIndex(1);
    else if (lang == "en")
        ui->languageCombo->setCurrentIndex(2);
    else
        ui->languageCombo->setCurrentIndex(0);

    ui->themeCombo->setCurrentIndex(m_configService->theme() == "dark" ? 1 : 0);
}

void SettingsDialog::onSaveClicked()
{
    m_configService->setAutoSyncEnabled(ui->autoSyncCheck->isChecked());
    m_configService->setSyncIntervalMinutes(ui->syncIntervalSpin->value());
    m_configService->setProxyUrl(ui->proxyUrlInput->text());
    m_configService->setAutoStart(ui->autoStartCheck->isChecked());
    m_configService->setMinimizeToTray(ui->minimizeToTrayCheck->isChecked());
    m_configService->setAutoStartMinimize(ui->autoStartMinimizeCheck->isChecked());
    m_configService->setFileTransferTimeoutSeconds(ui->timeoutSpin->value());

    int langIndex = ui->languageCombo->currentIndex();
    m_configService->setLanguage(
        langIndex == 1 ? "zh-CN" : (langIndex == 2 ? "en" : "auto"));

    m_configService->setTheme(ui->themeCombo->currentIndex() == 1 ? "dark" : "light");

    accept();
}

void SettingsDialog::onCancelClicked()
{
    reject();
}
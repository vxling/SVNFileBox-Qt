#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>

class ConfigService;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(ConfigService *configService, QWidget *parent = nullptr);
    ~SettingsDialog() override;

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    void loadSettings();

    ConfigService *m_configService = nullptr;

    QCheckBox *m_autoSyncCheck;
    QSpinBox *m_syncIntervalSpin;
    QLineEdit *m_proxyUrlInput;
    QCheckBox *m_autoStartCheck;
    QCheckBox *m_minimizeToTrayCheck;
    QCheckBox *m_autoStartMinimizeCheck;
    QSpinBox *m_timeoutSpin;
    QComboBox *m_languageCombo;
    QComboBox *m_themeCombo;
};
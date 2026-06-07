#pragma once
#include <QDialog>

class ConfigService;

namespace Ui {
class SettingsDialog;
}

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
    Ui::SettingsDialog *ui = nullptr;
    ConfigService *m_configService = nullptr;

    void loadSettings();
};
#pragma once
#include <QDialog>

class ConfigService;

namespace SVNFileBox {
class RepoGlobalManager;
}

namespace Ui {
class AddLocalDialog;
}

class AddLocalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddLocalDialog(SVNFileBox::RepoGlobalManager *globalManager,
                            ConfigService *configService,
                            QWidget *parent = nullptr);
    ~AddLocalDialog() override;

signals:
    void localRepoAdded(const QString &name, const QString &path);

private slots:
    void onBrowseClicked();
    void onConfirmClicked();
    void onCancelClicked();

private:
    Ui::AddLocalDialog *ui = nullptr;
    SVNFileBox::RepoGlobalManager *m_globalManager = nullptr;
    ConfigService *m_configService = nullptr;
};
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class ConfigService;

namespace SVNFileBox {
class RepoGlobalManager;
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
    SVNFileBox::RepoGlobalManager *m_globalManager = nullptr;
    ConfigService *m_configService = nullptr;

    QLineEdit *m_pathInput;
    QPushButton *m_browseBtn;
    QLabel *m_statusLabel;
};
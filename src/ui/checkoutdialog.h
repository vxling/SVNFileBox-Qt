#pragma once
#include <QDialog>

namespace SVNFileBox {
class RepoGlobalManager;
}

namespace Ui {
class CheckoutDialog;
}

class CheckoutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CheckoutDialog(SVNFileBox::RepoGlobalManager *globalManager, QWidget *parent = nullptr);
    ~CheckoutDialog() override;

signals:
    void checkoutCompleted(const QString &name, const QString &path);

private slots:
    void onBrowseFolder();
    void onConfirmClicked();
    void onCancelClicked();

private:
    Ui::CheckoutDialog *ui = nullptr;
    SVNFileBox::RepoGlobalManager *m_globalManager = nullptr;
};
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../services/repoglobalmanager.h"

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
    SVNFileBox::RepoGlobalManager *m_globalManager = nullptr;

    QLineEdit *m_nameInput;
    QLineEdit *m_urlInput;
    QLineEdit *m_userInput;
    QLineEdit *m_passInput;
    QLineEdit *m_folderInput;
    QPushButton *m_browseBtn;
    QLabel *m_statusLabel;
};
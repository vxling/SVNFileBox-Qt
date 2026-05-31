#pragma once
#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QWindow>

class TrayManager : public QObject
{
    Q_OBJECT

public:
    explicit TrayManager(QObject *parent = nullptr);
    ~TrayManager();

public slots:
    void show();
    void hide();
    void showMessage(const QString &title, const QString &msg, int iconType = 0);

signals:
    void showWindowRequested();
    void hideWindowRequested();
    void exitRequested();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onShow();
    void onHide();
    void onExit();

private:
    QWindow *m_window = nullptr;
    QSystemTrayIcon m_trayIcon;
    QMenu m_menu;
    QAction *m_showAction = nullptr;
    QAction *m_hideAction = nullptr;
    QAction *m_syncAction = nullptr;
    QAction *m_exitAction = nullptr;
    bool m_autoHide = true;
};

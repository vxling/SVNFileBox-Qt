#include "traymanager.h"
#include <QDebug>
#include <QApplication>

TrayManager::TrayManager(QObject *parent)
    : QObject(parent)
    , m_trayIcon(this)
{
    // Build context menu
    m_showAction = new QAction(tr("打开 SVNFileBox"), this);
    m_hideAction = new QAction(tr("隐藏到托盘"), this);
    m_syncAction = new QAction(tr("立即同步"), this);
    m_exitAction = new QAction(tr("退出"), this);

    m_menu.addAction(m_showAction);
    m_menu.addAction(m_hideAction);
    m_menu.addSeparator();
    m_menu.addAction(m_syncAction);
    m_menu.addSeparator();
    m_menu.addAction(m_exitAction);

    m_trayIcon.setContextMenu(&m_menu);
    m_trayIcon.setToolTip(QStringLiteral("SVNFileBox"));

    // Connect signals
    connect(m_showAction, SIGNAL(triggered()), SLOT(onShow()));
    connect(m_hideAction, SIGNAL(triggered()), SLOT(onHide()));
    connect(m_exitAction, SIGNAL(triggered()), SLOT(onExit()));
    connect(&m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(onTrayActivated(QSystemTrayIcon::ActivationReason)));

    // Use system icon if available
    if (m_trayIcon.supportsMessages()) {
        qDebug() << "System tray: supports messages";
    }
}

TrayManager::~TrayManager() = default;

void TrayManager::show() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray not available on this platform";
        return;
    }
    m_trayIcon.show();
}

void TrayManager::hide() {
    m_trayIcon.hide();
}

void TrayManager::showMessage(const QString &title, const QString &msg, int iconType) {
    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(iconType);
    // Fallback to NoIcon if out of range (Linux may not support all)
    if (iconType < 0 || iconType > 3) icon = QSystemTrayIcon::NoIcon;
    m_trayIcon.showMessage(title, msg, icon, 3000);
}

void TrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        emit showWindowRequested();
    }
}

void TrayManager::onShow() {
    emit showWindowRequested();
}

void TrayManager::onHide() {
    emit hideWindowRequested();
}

void TrayManager::onExit() {
    emit exitRequested();
}

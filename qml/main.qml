import QtQuick
import QtQuick.Controls
import QtQuick.Window
import SVNFileBox.System

ApplicationWindow {
    id: root
    visible: true
    color: "#F0F4F8"

    width: 1100
    height: 580
    minimumWidth: 900
    minimumHeight: 550

    // TrayManager 信号连接
    Connections {
        target: trayManager

        function onShowWindowRequested() {
            root.show()
            root.raise()
            root.requestActivate()
        }

        function onExitRequested() {
            trayManager.hide()
            Qt.quit()
        }
    }

    // 窗口关闭 → 隐藏到托盘（而非退出）
    onClosing: function(close) {
        if (root.visible) {
            close.accepted = false
            root.hide()
            trayManager.showMessage("SVNFileBox", "已最小化到托盘", 1)
        }
    }

    Component.onCompleted: {
        trayManager.show()
    }

    // MainWindow is now an Item, load it directly
    Loader {
        id: mainLoader
        anchors.fill: parent
        source: "qrc:/qml/pages/MainWindow.qml"
    }
}

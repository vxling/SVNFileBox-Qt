import QtQuick
import QtQuick.Controls
import QtQuick.Window

ApplicationWindow {
    id: root
    visible: true
    color: "#F0F4F8"

    // 窗口默认尺寸
    width: 1100
    height: 580
    minimumWidth: 900
    minimumHeight: 550

    // 居中启动
    x: Screen.desktopAvailableWidth / 2 - width / 2
    y: Screen.desktopAvailableHeight / 2 - height / 2

    // 加载主页面
    SwipeView {
        id: swipeView
        anchors.fill: parent
        interactive: false
        currentIndex: 0

        Loader {
            source: "qrc:/qml/pages/MainWindow.qml"
        }
    }
}

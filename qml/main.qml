import QtQuick
import QtQuick.Controls
import QtQuick.Window

ApplicationWindow {
    id: root
    visible: true
    color: "#F0F4F8"

    width: 1100
    height: 580
    minimumWidth: 900
    minimumHeight: 550

    // 让 Qt 自动居中，不手动算坐标（避免最大化时 title bar 偏上）
    Component.onCompleted: root.centerOnScreen()

    // MainWindow is now an Item, load it directly
    Loader {
        id: mainLoader
        anchors.fill: parent
        source: "qrc:/qml/pages/MainWindow.qml"
    }
}

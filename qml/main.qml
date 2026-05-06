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

    x: Screen.desktopAvailableWidth / 2 - width / 2
    y: Screen.desktopAvailableHeight / 2 - height / 2

    // MainWindow is now an Item, load it directly
    Loader {
        id: mainLoader
        anchors.fill: parent
        source: "qrc:/qml/pages/MainWindow.qml"
    }
}

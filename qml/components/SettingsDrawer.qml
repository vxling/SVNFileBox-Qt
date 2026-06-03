import QtQuick
import QtQuick.Controls

Drawer {
    id: settingsDrawer
    edge: Qt.RightEdge
    width: parent.width * 0.5
    height: parent.height - 36
    y: 0
    position: 0

    SettingsDrawerContent {
        anchors.fill: parent
        onSaveClicked: saveSettings()
        onCloseClicked: settingsDrawer.close()
    }
}

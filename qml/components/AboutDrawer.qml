import QtQuick
import QtQuick.Controls

Drawer {
    id: aboutDrawer
    edge: Qt.RightEdge
    width: parent.width * 0.5
    height: parent.height - 36
    y: 0
    position: 0

    AboutDrawerContent {
        anchors.fill: parent
        onCloseClicked: aboutDrawer.close()
    }
}

import QtQuick
import QtQuick.Controls

Drawer {
    id: syncRecordsDrawer
    edge: Qt.RightEdge
    width: parent.width * 0.6
    height: parent.height - 36
    y: 0
    position: 0

    SyncRecordsDrawerContent {
        anchors.fill: parent
        onClearClicked: syncRecordService.clearRecords()
        onCloseClicked: syncRecordsDrawer.close()
    }
}

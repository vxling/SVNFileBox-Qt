import QtQuick
import QtQuick.Controls

Popup {
    id: confirmDeleteDialog
    property string fileToDelete: ""
    property string fileName: ""
    anchors.centerIn: parent
    width: 360
    height: confirmDeleteDialogContent.height + 40
    modal: true
    closePolicy: Popup.NoAutoClose
    padding: 20

    Column {
        id: confirmDeleteDialogContent
        spacing: 16

        Label {
            text: qsTr("确认删除")
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        Label {
            text: qsTr("确定要删除 \"") + confirmDeleteDialog.fileName + qsTr("\" 吗？此操作不可撤销。")
            wrapMode: Text.WordWrap
            width: 300
        }

        Row {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("取消")
                onClicked: confirmDeleteDialog.close()
            }

            Button {
                text: qsTr("确定")
                highlighted: true
                onClicked: {
                    confirmDeleteDialog.close()
                    globalManager.activeManager.activeExecutor().executeLocalWrite(6, confirmDeleteDialog.fileToDelete)
                }
            }
        }
    }
}

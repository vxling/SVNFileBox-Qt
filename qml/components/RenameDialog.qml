import QtQuick
import QtQuick.Controls

Popup {
    id: renameDialog
    anchors.centerIn: parent
    width: 360
    height: renameDialogContent.height + 40
    modal: true
    closePolicy: Popup.NoAutoClose
    padding: 20

    property string oldPath: ""
    property string oldName: ""
    property alias newNameField: renameNameInput

    Column {
        id: renameDialogContent
        spacing: 16

        Label {
            text: qsTr("重命名")
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        TextField {
            id: renameNameInput
            placeholderText: qsTr("新名称")
            width: 300
            implicitHeight: 36
        }

        Row {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("取消")
                onClicked: renameDialog.close()
            }

            Button {
                text: qsTr("确定")
                highlighted: true
                onClicked: {
                    if (renameNameInput.text.trim() !== "" && renameNameInput.text !== renameDialog.oldName) {
                        var newPath = currentPath + "/" + renameNameInput.text.trim()
                        globalManager.activeManager.activeExecutor().executeLocalWrite(5, newPath, renameDialog.oldPath)
                    }
                    renameDialog.close()
                }
            }
        }
    }
}

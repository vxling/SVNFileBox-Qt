import QtQuick
import QtQuick.Controls

Popup {
    id: newFolderDialog
    anchors.centerIn: parent
    width: 360
    height: newFolderDialogContent.height + 40
    modal: true
    closePolicy: Popup.NoAutoClose
    padding: 20

    property alias newNameField: newFolderNameInput

    Column {
        id: newFolderDialogContent
        spacing: 16

        Label {
            text: qsTr("新建文件夹")
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        TextField {
            id: newFolderNameInput
            placeholderText: qsTr("新文件夹")
            width: 300
            implicitHeight: 36
        }

        Row {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("取消")
                onClicked: newFolderDialog.close()
            }

            Button {
                text: qsTr("确定")
                highlighted: true
                onClicked: {
                    if (newFolderNameInput.text.trim() !== "") {
                        var newPath = currentPath + "/" + newFolderNameInput.text.trim()
                        if (fileModel.createDirectory(newPath)) {
                            globalManager.activeManager.activeExecutor().executeLocalWrite(8, newPath)
                        }
                    }
                    newFolderDialog.close()
                }
            }
        }
    }
}

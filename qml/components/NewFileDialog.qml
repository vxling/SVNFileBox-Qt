import QtQuick
import QtQuick.Controls

Popup {
    id: newFileDialog
    anchors.centerIn: parent
    width: 360
    height: newFileDialogContent.height + 40
    modal: true
    closePolicy: Popup.NoAutoClose
    padding: 20

    property string ext: "txt"
    property alias newNameField: newFileNameInput

    Column {
        id: newFileDialogContent
        spacing: 16

        Label {
            text: qsTr("新建文件")
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        TextField {
            id: newFileNameInput
            placeholderText: qsTr("新建文件")
            width: 300
            implicitHeight: 36
        }

        Label {
            text: qsTr("类型: .") + newFileDialog.ext
            color: "#666"
        }

        Row {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("取消")
                onClicked: newFileDialog.close()
            }

            Button {
                text: qsTr("确定")
                highlighted: true
                onClicked: {
                    var name = newFileNameInput.text.trim()
                    if (name === "") {
                        newFileDialog.close()
                        return
                    }
                    if (!name.endsWith("." + newFileDialog.ext))
                        name += "." + newFileDialog.ext
                    var fullPath = currentPath + "/" + name
                    if (fileModel.createFile(fullPath)) {
                        globalManager.activeManager.activeExecutor().executeLocalWrite(4, fullPath)
                    }
                    newFileDialog.close()
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls

Popup {
    id: renameRepoDialog
    anchors.centerIn: parent
    width: 380
    height: renameRepoDialogContent.height + 40
    modal: true
    closePolicy: Popup.NoAutoClose
    padding: 20

    property int index: -1
    property string oldName: ""
    property string statusText: ""
    property alias newNameField: renameRepoNameInput

    Column {
        id: renameRepoDialogContent
        spacing: 12
        width: 340

        Label {
            text: qsTr("重命名仓库")
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        Label {
            text: qsTr("原名称：") + renameRepoDialog.oldName
            font.pixelSize: 12
            color: "#666666"
        }

        TextField {
            id: renameRepoNameInput
            placeholderText: qsTr("新名称")
            width: 340
            implicitHeight: 36
            selectByMouse: true
        }

        Label {
            text: renameRepoDialog.statusText
            color: "#E53935"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            width: 340
            visible: text !== ""
        }

        Row {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("取消")
                onClicked: renameRepoDialog.close()
            }

            Button {
                text: qsTr("确定")
                highlighted: true
                onClicked: {
                    var newName = renameRepoNameInput.text.trim()
                    if (newName === "" || newName === renameRepoDialog.oldName) {
                        renameRepoDialog.statusText = "新名称无效或与原名称相同"
                        return
                    }
                    // Duplicate check against other rows
                    for (var i = 0; i < repoListModel.count; i++) {
                        if (i !== renameRepoDialog.index && repoListModel.get(i).name === newName) {
                            renameRepoDialog.statusText = "已存在同名仓库"
                            return
                        }
                    }
                    globalManager.renameRepoByName(renameRepoDialog.oldName, newName)
                    renameRepoDialog.close()
                }
            }
        }
    }
}

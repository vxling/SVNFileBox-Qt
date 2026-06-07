import QtQuick
import QtQuick.Controls

Popup {
    id: editRepoDialog
    anchors.centerIn: parent
    width: 420
    height: editRepoDialogContent.height + 40
    modal: true
    closePolicy: Popup.NoAutoClose
    padding: 20

    property int index: -1
    property string name: ""
    property string statusText: ""
    property alias urlField: editRepoUrlInput

    Column {
        id: editRepoDialogContent
        spacing: 12
        width: 380

        Label {
            text: qsTr("修改仓库 URL")
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        Label {
            text: qsTr("仓库：") + editRepoDialog.name
            font.pixelSize: 12
            color: "#666666"
        }

        TextField {
            id: editRepoUrlInput
            placeholderText: qsTr("新 URL (例如 https://svn.example.com/repo)")
            width: 380
            implicitHeight: 36
            selectByMouse: true
        }

        Label {
            text: qsTr("注：仅修改本地记录的 URL；如需重新定位工作副本，请使用 svn switch --relocate。")
            font.pixelSize: 10
            color: "#999999"
            wrapMode: Text.WordWrap
            width: 380
        }

        Label {
            text: editRepoDialog.statusText
            color: "#E53935"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            width: 380
            visible: text !== ""
        }

        Row {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("取消")
                onClicked: editRepoDialog.close()
            }

            Button {
                text: qsTr("保存")
                highlighted: true
                onClicked: {
                    var newUrl = editRepoUrlInput.text.trim()
                    if (newUrl === "") {
                        editRepoDialog.statusText = "URL 不能为空"
                        return
                    }
                    globalManager.updateRepoUrlByName(editRepoDialog.name, newUrl)
                    editRepoDialog.close()
                }
            }
        }
    }
}

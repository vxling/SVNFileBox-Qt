import QtQuick
import QtQuick.Controls

Popup {
    id: conflictDialog
    anchors.centerIn: parent
    width: 500
    height: 320
    modal: true
    closePolicy: Popup.NoAutoClose
    padding: 20

    property var conflictFileList: []

    Column {
        anchors.fill: parent
        spacing: 12

        Label {
            text: qsTr("冲突检测")
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        Label {
            text: qsTr("检测到文件冲突，请手动解决：")
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        ListView {
            id: conflictList
            height: 180
            width: parent.width
            anchors.left: parent.left
            anchors.right: parent.right
            model: conflictDialog.conflictFileList
            interactive: false
            clip: true
            delegate: Label {
                text: modelData
                font.pixelSize: 12
                color: "#E53935"
                wrapMode: Text.WordWrap
            }
        }

        Button {
            text: qsTr("确定")
            anchors.horizontalCenter: parent.horizontalCenter
            highlighted: true
            onClicked: conflictDialog.close()
        }
    }
}

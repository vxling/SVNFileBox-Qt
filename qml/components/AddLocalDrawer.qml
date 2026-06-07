import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform

Drawer {
    id: addLocalDrawer
    edge: Qt.RightEdge
    width: parent.width * 0.5
    height: parent.height - 36
    y: 0
    position: 0

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 40
            spacing: 16

            Label {
                text: qsTr("添加本地仓库")
                font.pixelSize: 20
                font.weight: Font.DemiBold
                color: "#1A1A2E"
            }

            Label {
                text: qsTr("选择一个已有的 SVN 工作副本目录")
                font.pixelSize: 13
                color: "#666666"
            }

            RowLayout {
                spacing: 12
                TextField {
                    id: localRepoPathInput
                    placeholderText: qsTr("选择本地 SVN 工作副本目录")
                    Layout.fillWidth: true
                    implicitHeight: 36
                    readOnly: true
                }
                Button {
                    text: qsTr("浏览...")
                    implicitWidth: 90
                    implicitHeight: 36
                    onClicked: localRepoDialog.open()
                }
            }

            FileDialog {
                id: localRepoDialog
                title: qsTr("选择 SVN 工作副本目录")
                modality: Qt.NonModal
                fileMode: FileDialog.Directory
                selectFolder: true
                folder: "file:///home/osuser"
                onAccepted: {
                    localRepoPathInput.text = localRepoDialog.folder
                }
            }

            RowLayout {
                spacing: 12
                Button {
                    text: qsTr("确认")
                    implicitWidth: 100
                    implicitHeight: 36
                    onClicked: doAddLocal()
                }
                Button {
                    text: qsTr("取消")
                    implicitWidth: 80
                    implicitHeight: 36
                    onClicked: addLocalDrawer.close()
                }
            }

            Item {
                Layout.fillHeight: true
            }

            Label {
                id: addLocalStatusLabel
                text: ""
                color: "#E53935"
                font.pixelSize: 12
            }
        }
    }
}

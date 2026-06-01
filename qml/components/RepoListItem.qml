import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property string repoName: ""
    property string repoPath: ""
    property string repoType: "Local"  // Local or Network
    property bool isSelected: false
    signal removeClicked()
    signal itemClicked()
    signal renameClicked()
    signal editUrlClicked()

    color: {
        if (isSelected) return "#E3F2FD"
        if (mouseArea.containsMouse) return "#E8EDF2"
        return "#F5F7FA"
    }
    border.color: isSelected ? "#1E88E5" : "transparent"
    border.width: 3
    radius: 6

    Behavior on color { ColorAnimation { duration: 120 } }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                repoContextMenu.popup()
            } else {
                root.itemClicked()
            }
        }
    }

    Menu {
        id: repoContextMenu
        MenuItem {
            text: "重命名..."
            onTriggered: root.renameClicked()
        }
        MenuItem {
            text: "修改 URL..."
            onTriggered: root.editUrlClicked()
        }
        MenuSeparator {}
        MenuItem {
            text: "移除仓库"
            onTriggered: root.removeClicked()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 4
        spacing: 10

        Label {
            text: root.repoType === "Network" ? "🌐" : "📂"
            font.pixelSize: 20
            Layout.alignment: Qt.AlignVCenter
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Label {
                text: root.repoName
                font.pixelSize: 13
                font.weight: Font.Medium
                color: "#1A1A2E"
                Layout.fillWidth: true
            }

            Label {
                text: root.repoPath
                font.pixelSize: 10
                color: "#888888"
                Layout.fillWidth: true
                elide: Text.ElideMiddle
                maximumLineCount: 1
            }
        }

        Button {
            text: "✕"
            implicitWidth: 24
            implicitHeight: 24
            background: Rectangle { color: "transparent" }
            contentItem: Label {
                text: control.text
                color: "#E53935"
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            hoverEnabled: true
            onClicked: root.removeClicked()
            Layout.alignment: Qt.AlignVCenter
        }
    }
}

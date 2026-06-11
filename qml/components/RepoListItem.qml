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
        if (isSelected) return "#D9EBFD"
        if (mouseArea.containsMouse) return "#EEF4FA"
        return "#FFFFFF"
    }

    // 左侧选中竖条
    Rectangle {
        id: leftBar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 3
        color: "#1E88E5"
        visible: isSelected
    }

    // 悬停时显示删除按钮
    property alias removeButton: removeBtn

    Behavior on color { ColorAnimation { duration: 120 } }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.leftMargin: 3  // 留出竖条空间
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
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
        anchors.leftMargin: 12
        anchors.rightMargin: 8
        anchors.right: parent.right
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

        // 删除按钮：默认隐藏，悬停或选中时显示
        Button {
            id: removeBtn
            text: "✕"
            implicitWidth: 24
            implicitHeight: 24
            visible: mouseArea.containsMouse || isSelected
            opacity: visible ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 150 } }

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
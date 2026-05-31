import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    property string name: ""
    property string fullPath: ""
    property bool isDirectory: false
    property string svnStatus: "Normal"
    property string fileSizeDisplay: ""
    property string lastModifiedDisplay: ""
    property bool isCurrentPath: false  // "返回上级目录" 行
    signal doubleClicked()
    signal contextMenuRequested(real x, real y)

    color: {
        if (isCurrentPath) return "#F8F9FA"
        if (mouseArea.containsMouse) return "#F0F4F8"
        return "transparent"
    }
    border.color: "#EEEEEE"
    border.width: 1
    radius: 4
    height: 44

    Behavior on color { ColorAnimation { duration: 100 } }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                root.contextMenuRequested(mouse.x, mouse.y)
            }
        }
        onDoubleClicked: if (!root.isCurrentPath) doubleClicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 12
        spacing: 0

        // 类型图标
        Item { implicitWidth: 40; implicitHeight: 44; Layout.alignment: Qt.AlignVCenter; Layout.minimumWidth: 40
            FileTypeIcon {
                anchors.centerIn: parent
                fileName: root.name
                isDirectory: root.isDirectory
            }
        }

        // 名称
        Label {
            text: root.isCurrentPath ? "← 返回上级目录" : root.name
            font.pixelSize: 13
            font.weight: root.isCurrentPath ? Font.Normal : Font.Medium
            color: root.isCurrentPath ? "#1E88E5" : "#1A1A2E"
            Layout.fillWidth: true
            Layout.minimumWidth: 280
            Layout.preferredWidth: 280
            elide: Text.ElideMiddle
            leftPadding: root.name === ".." ? 0 : 8
        }

        // 状态徽章
        Item { implicitWidth: 70; implicitHeight: 44; Layout.alignment: Qt.AlignVCenter; Layout.minimumWidth: 70
            StatusBadge {
                anchors.horizontalCenter: parent.horizontalCenter
                svnStatus: root.isCurrentPath ? "Hidden" : root.svnStatus
            }
        }

        // 大小
        Label {
            text: root.fileSizeDisplay
            font.pixelSize: 12
            color: "#666666"
            Layout.minimumWidth: 90
            Layout.preferredWidth: 90
            horizontalAlignment: Text.AlignRight
        }

        // 修改时间
        Label {
            text: root.lastModifiedDisplay
            font.pixelSize: 12
            color: "#666666"
            Layout.minimumWidth: 140
            Layout.preferredWidth: 140
        }
    }
}

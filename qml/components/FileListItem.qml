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
    property string typeDisplay: ""
    property bool isCurrentPath: false  // "返回上级目录" 行
    signal doubleClicked()
    signal contextMenuRequested(real x, real y)

    // 整行背景：选中=淡蓝，悬停=更淡蓝，返回上级=浅灰背景
    color: {
        if (root.isCurrentPath) return "#F5F5F5"
        if (mouseArea.containsMouse) return "#E8F4FD"
        return "#FFFFFF"
    }

    // 移除行边框
    border.color: "#FFFFFF"
    border.width: 0

    // 圆角
    radius: 4
    height: 44

    Behavior on color { ColorAnimation { duration: 100 } }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.margins: 2
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

        // 状态徽章（圆形图标）
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

        // 文件类型
        Label {
            text: root.typeDisplay
            font.pixelSize: 12
            color: "#888888"
            Layout.minimumWidth: 80
            Layout.preferredWidth: 80
            horizontalAlignment: Text.AlignLeft
        }
    }
}
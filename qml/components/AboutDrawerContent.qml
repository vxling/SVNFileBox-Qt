import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal closeClicked()

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 40
            spacing: 16

            Label {
                text: "关于 SVNFileBox"
                font.pixelSize: 22
                font.weight: Font.DemiBold
                color: "#1A1A2E"
            }

            Label {
                text: "版本 1.0.0"
                font.pixelSize: 14
                color: "#666666"
            }

            Label {
                text: "SVN 版 Dropbox。基于 Qt 6.5.3 + QML + CMake 重写。"
                font.pixelSize: 13
                color: "#333333"
                wrapMode: Text.WordWrap
                Layout.preferredWidth: 400
            }

            Label {
                text: "参考项目：C# WPF SVNFileBox"
                font.pixelSize: 13
                color: "#888888"
            }

            Label {
                text: "© 2026 vxling"
                font.pixelSize: 12
                color: "#AAAAAA"
                Layout.topMargin: 20
            }

            Item { Layout.fillHeight: true }

            Button {
                text: "关闭"
                implicitWidth: 80; implicitHeight: 36
                onClicked: closeClicked()
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal clearClicked()
    signal closeClicked()

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "同步记录 (" + syncRecordService.recordCount + ")"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: "#1A1A2E"
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "清空"
                    implicitWidth: 60; implicitHeight: 30
                    onClicked: clearClicked()
                }
                Button {
                    text: "关闭"
                    implicitWidth: 60; implicitHeight: 30
                    onClicked: root.closeClicked()
                }
            }

            Rectangle { height: 1; color: "#E8E8E8"; Layout.fillWidth: true }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: "#F8F9FA"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    spacing: 0

                    Label { Layout.minimumWidth: 160; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "时间" }
                    Label { Layout.minimumWidth: 120; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "仓库" }
                    Label { Layout.minimumWidth: 280; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "文件" }
                    Label { Layout.minimumWidth: 100; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "操作" }
                    Label { Layout.minimumWidth: 80; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "结果" }
                    Label { Layout.fillWidth: true; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "消息" }
                }
            }

            ListView {
                id: syncRecordListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                cacheBuffer: 200
                model: syncRecordService

                delegate: Rectangle {
                    width: syncRecordListView.width
                    height: 36
                    color: parent.ListView.isCurrentItem ? "#F8F9FA" : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        spacing: 0

                        Label { Layout.minimumWidth: 160; text: model.timestamp; font.pixelSize: 12; color: "#333333" }
                        Label { Layout.minimumWidth: 120; text: model.repoName; font.pixelSize: 12; color: "#333333" }
                        Label { Layout.minimumWidth: 280; text: model.filePath; font.pixelSize: 12; color: "#333333"; elide: Text.ElideMiddle }
                        Label { Layout.minimumWidth: 100; text: model.operation; font.pixelSize: 12; color: "#1E88E5" }
                        Label { Layout.minimumWidth: 80; text: model.result; font.pixelSize: 12; color: model.result === "Success" ? "#4CAF50" : "#E53935" }
                        Label { Layout.fillWidth: true; text: model.message; font.pixelSize: 12; color: "#888888"; elide: Text.ElideRight }
                    }
                }
            }
        }
    }
}

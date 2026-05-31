import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // 暴露给 MainWindow 读取表单值
    property alias syncIntervalText: syncIntervalInput.text
    property alias proxyUrlText: proxyUrlInput.text
    property alias retentionDaysText: retentionDaysInput.text
    property alias autoStartChecked: autoStartCheck.checked
    property alias minimizeToTrayChecked: minimizeToTrayCheck.checked
    property alias statusText: settingsStatusLabel.text

    signal saveClicked()

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 40
            spacing: 16

            Label {
                text: "设置"
                font.pixelSize: 20
                font.weight: Font.DemiBold
                color: "#1A1A2E"
            }

            GridLayout {
                columns: 2
                rowSpacing: 16
                columnSpacing: 12

                Label { text: "同步周期（分钟）:"; Layout.alignment: Qt.AlignRight; font.pixelSize: 13 }
                TextField {
                    id: syncIntervalInput
                    placeholderText: "1"
                    Layout.minimumWidth: 200
                    text: configService.syncIntervalMinutes
                }

                Label { text: "代理地址:"; Layout.alignment: Qt.AlignRight; font.pixelSize: 13 }
                TextField {
                    id: proxyUrlInput
                    placeholderText: "http://proxy:8080"
                    Layout.minimumWidth: 200
                    text: configService.proxyUrl
                }

                Label { text: "同步记录保留（天）:"; Layout.alignment: Qt.AlignRight; font.pixelSize: 13 }
                TextField {
                    id: retentionDaysInput
                    placeholderText: "30"
                    Layout.minimumWidth: 200
                    text: configService.syncRecordRetentionDays
                }

                Label { text: "开机启动:"; Layout.alignment: Qt.AlignRight; font.pixelSize: 13 }
                CheckBox {
                    id: autoStartCheck
                    checked: configService.autoStart
                }

                Label { text: "最小化到托盘:"; Layout.alignment: Qt.AlignRight; font.pixelSize: 13 }
                CheckBox {
                    id: minimizeToTrayCheck
                    checked: configService.minimizeToTray
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                spacing: 12
                Button {
                    text: "保存"
                    implicitWidth: 100; implicitHeight: 36
                    onClicked: root.saveClicked()
                }
                Button {
                    text: "关闭"
                    implicitWidth: 80; implicitHeight: 36
                    onClicked: parent.parent.parent.close()
                }
            }

            Label { id: settingsStatusLabel; text: ""; color: "#4CAF50"; font.pixelSize: 12 }
        }
    }
}

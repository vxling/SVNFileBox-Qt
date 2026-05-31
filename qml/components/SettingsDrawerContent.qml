import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // 暴露给 MainWindow 读取表单值
    property alias autoSyncEnabledChecked: autoSyncCheck.checked
    property alias syncIntervalValue: syncIntervalSlider.value
    property alias themeIndex: themeCombo.currentIndex
    property alias languageIndex: languageCombo.currentIndex
    property alias proxyUrlText: proxyUrlInput.text
    property alias autoStartChecked: autoStartCheck.checked
    property alias autoStartMinimizeChecked: autoStartMinimizeCheck.checked
    property alias minimizeToTrayChecked: minimizeToTrayCheck.checked
    property alias fileTransferTimeoutValue: fileTransferTimeoutSlider.value
    property alias statusText: settingsStatusLabel.text

    signal saveClicked()
    signal closeClicked()

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

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Column {
                    spacing: 16
                    width: parent.parent.width

                    // === Auto Sync ===
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "自动同步"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Label {
                            text: "启用后自动同步文件变更（不可关闭）"
                            font.pixelSize: 11
                            color: "#666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Item { Layout.fillWidth: true }
                        CheckBox {
                            id: autoSyncCheck
                            checked: configService.autoSyncEnabled
                            enabled: false // WPF 里固定灰置
                        }
                    }

                    // === Sync Interval (Slider 1-10) ===
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "同步周期（分钟）"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Label {
                            text: "服务器轮询检查的间隔（1-10分钟）"
                            font.pixelSize: 11
                            color: "#666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Item { Layout.fillWidth: true }
                    }
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Slider {
                            id: syncIntervalSlider
                            from: 1
                            to: 10
                            stepSize: 1
                            value: configService.syncIntervalMinutes
                            implicitWidth: 200
                        }
                        Label {
                            text: syncIntervalSlider.value + " 分钟"
                            font.pixelSize: 12
                            Layout.minimumWidth: 60
                        }
                    }

                    // === Theme ===
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "主题"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Label {
                            text: "界面外观颜色模式（需重启生效）"
                            font.pixelSize: 11
                            color: "#666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Item { Layout.fillWidth: true }
                        ComboBox {
                            id: themeCombo
                            implicitWidth: 120
                            // 0=跟随系统, 1=Light, 2=Dark
                            currentIndex: configService.theme === "light" ? 1 : (configService.theme === "dark" ? 2 : 0)
                            model: ["跟随系统", "Light", "Dark"]
                        }
                    }

                    // === Language ===
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "语言"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Label {
                            text: "界面显示语言"
                            font.pixelSize: 11
                            color: "#666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Item { Layout.fillWidth: true }
                        ComboBox {
                            id: languageCombo
                            implicitWidth: 120
                            // 0=auto, 1=zh, 2=en
                            currentIndex: configService.language === "zh" ? 1 : (configService.language === "en" ? 2 : 0)
                            model: ["跟随系统", "中文", "English"]
                        }
                    }

                    // === Proxy ===
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "代理地址"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Label {
                            text: "HTTP 代理（留空则不使用代理）"
                            font.pixelSize: 11
                            color: "#666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                    TextField {
                        id: proxyUrlInput
                        placeholderText: "http://proxy:8080"
                        Layout.fillWidth: true
                        text: configService.proxyUrl
                    }

                    // === Auto Start ===
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "开机启动"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Label {
                            text: "Windows 启动时自动运行"
                            font.pixelSize: 11
                            color: "#666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Item { Layout.fillWidth: true }
                        CheckBox {
                            id: autoStartCheck
                            checked: configService.autoStart
                            onCheckedChanged: autoStartMinimizeCheck.enabled = checked
                        }
                    }

                    // === Auto Start Minimize ===
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "启动最小化"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Label {
                            text: "开机启动时最小化到托盘"
                            font.pixelSize: 11
                            color: "#666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Item { Layout.fillWidth: true }
                        CheckBox {
                            id: autoStartMinimizeCheck
                            checked: configService.autoStartMinimize
                            enabled: autoStartCheck.checked
                        }
                    }

                    // === Minimize to Tray ===
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "最小化到托盘"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Label {
                            text: "关闭窗口时最小化到系统托盘"
                            font.pixelSize: 11
                            color: "#666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Item { Layout.fillWidth: true }
                        CheckBox {
                            id: minimizeToTrayCheck
                            checked: configService.minimizeToTray
                        }
                    }

                    // === File Transfer Timeout (Slider 30-600) ===
                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Label {
                            text: "文件传输超时"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Label {
                            text: "文件传输过程中超过此时间无活动则判定为卡死"
                            font.pixelSize: 11
                            color: "#666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                    RowLayout {
                        spacing: 12
                        Slider {
                            id: fileTransferTimeoutSlider
                            from: 30
                            to: 600
                            stepSize: 30
                            value: configService.fileTransferTimeoutSeconds
                            implicitWidth: 200
                        }
                        Label {
                            text: fileTransferTimeoutSlider.value + " 秒"
                            font.pixelSize: 12
                            Layout.minimumWidth: 60
                        }
                    }
                }
            }

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
                    onClicked: root.closeClicked()
                }
            }

            Label { id: settingsStatusLabel; text: ""; color: "#4CAF50"; font.pixelSize: 12 }
        }
    }
}
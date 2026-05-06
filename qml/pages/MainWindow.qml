import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import SVNFileBox.SVN
import SVNFileBox.Sync
import SVNFileBox.Config
import SVNFileBox.Models

Window {
    id: mainWindow
    width: 900
    height: 650
    visible: true
    title: "SVN FileBox"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        // 工具栏
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: "选择本地文件夹"
                onClicked: selectFolderDialog.open()
            }

            Label {
                text: localPathLabel.text
                Layout.fillWidth: true
            }

            Button {
                text: "设置"
                onClicked: settingsDrawer.open()
            }

            Button {
                text: syncButton.text
                enabled: svnClient && configService.remoteUrl !== ""
                onClicked: toggleSync()
            }
        }

        // 状态栏
        Label {
            id: statusLabel
            text: "状态: " + syncEngine.status()
            Layout.fillWidth: true
            font.italic: true
            color: "gray"
        }

        // 文件列表
        ListView {
            id: fileListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: fileModel

            delegate: fileDelegate
            clip: true
            visible: fileModel.count > 0
        }

        // 空状态
        Label {
            text: "暂无文件，请在左侧添加文件夹"
            anchors.centerIn: parent
            visible: fileModel.count === 0
            font.italic: true
            color: "gray"
        }
    }

    // 文件列表项
    Component {
        id: fileDelegate
        Rectangle {
            width: fileListView.width
            height: 40
            border.color: "#eee"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8

                // 状态图标
                Image {
                    source: statusIcon(statusRole)
                    width: 20
                    height: 20
                }

                Label {
                    text: name
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: path
                    color: "gray"
                }

                Label {
                    text: statusRole
                    color: statusColor(statusRole)
                }
            }
        }
    }

    function statusIcon(status) {
        switch(status) {
            case "synced": return "qrc:/qml/assets/synced.png";
            case "modified": return "qrc:/qml/assets/modified.png";
            case "new": return "qrc:/qml/assets/new.png";
            case "deleted": return "qrc:/qml/assets/deleted.png";
            default: return "qrc:/qml/assets/unknown.png";
        }
    }

    function statusColor(status) {
        switch(status) {
            case "synced": return "green";
            case "modified": return "orange";
            case "new": return "blue";
            case "deleted": return "red";
            default: return "gray";
        }
    }

    // 设置抽屉
    Drawer {
        id: settingsDrawer
        width: 300
        height: parent.height
        edge: Qt.RightEdge

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15

            Label {
                text: "设置"
                font.bold: true
                font.pixelSize: 18
            }

            TextField {
                id: remoteUrlInput
                placeholderText: "svn://服务器地址/仓库路径"
                text: configService.remoteUrl()
                onEditingFinished: saveConfig()
            }

            TextField {
                id: usernameInput
                placeholderText: "SVN 用户名"
                text: configService.username()
                onEditingFinished: saveConfig()
            }

            Button {
                text: "保存"
                onClicked: saveConfig()
            }

            Item { Layout.fillHeight: true }
        }
    }

    function saveConfig() {
        var cfg = configService.loadConfig();
        cfg.remoteUrl = remoteUrlInput.text;
        cfg.username = usernameInput.text;
        cfg.localPath = localPathLabel.text;
        configService.saveConfig(cfg);
    }

    function toggleSync() {
        if (syncEngine.status() === "running") {
            syncEngine.stopSync();
            syncButton.text = "开始同步";
        } else {
            syncEngine.startSync(localPathLabel.text, configService.remoteUrl());
            syncButton.text = "停止同步";
        }
    }

    Label {
        id: localPathLabel
        text: configService.localPath()
        visible: false
    }

    FileModel {
        id: fileModel
    }

    FileDialog {
        id: selectFolderDialog
        title: "选择本地 SVN 文件夹"
        folder: shortcuts.home
        onAccepted: {
            localPathLabel.text = folder;
            var cfg = configService.loadConfig();
            cfg.localPath = folder;
            configService.saveConfig(cfg);
        }
    }

    Button {
        id: syncButton
        text: "开始同步"
        visible: false
    }
}

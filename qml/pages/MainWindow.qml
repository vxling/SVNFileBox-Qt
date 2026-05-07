import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.platform
import SVNFileBox.SVN
import SVNFileBox.Sync
import SVNFileBox.Config
import SVNFileBox.Models
import "../components"

Item {
    id: mainWindow

    // ================================================================
    // SyncEngine 信号连接
    // ================================================================
    Connections {
        target: syncEngine
        function onSyncNotification(message) {
            statusBarText.text = message
            syncIndicator.running = false
        }
        function onFilesChanged() {
            fileModel.load(currentPath)
            syncIndicator.running = false
        }
        function onConflictDetected(files) {
            conflictDialog.conflictFileList = files
            conflictDialog.open()
            syncIndicator.running = false
        }
        function onSyncStarted() {
            statusBarText.text = "同步中..."
            syncIndicator.running = true
        }
    }

    // ================================================================
    // 启动时加载仓库列表
    // ================================================================
    Component.onCompleted: {
        var repos = configService.repositories()
        for (var i = 0; i < repos.length; i++) {
            repoListModel.append(repos[i])
        }
        // 找到 active repo 并选中
        for (var j = 0; j < repoListModel.count; j++) {
            if (repoListModel.get(j).isSelected) {
                selectRepo(j)
                return
            }
        }
        // 没有 active repo 且有仓库，选第一个
        if (repoListModel.count > 0) {
            selectRepo(0)
        }
    }

    // ================================================================
    // 第一层：主布局（左右栏 + 状态栏）
    // ================================================================
    RowLayout {
        anchors.fill: parent
        anchors.bottomMargin: 36  // 为状态栏留空间
        spacing: 0

        // ---------- 左侧栏 ----------
        Rectangle {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            color: "#FFFFFF"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 0

                Label {
                    text: "仓库列表"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: "#1A1A2E"
                    Layout.bottomMargin: 8
                }

                ListView {
                    id: repoListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: ListModel { id: repoListModel }
                    delegate: repoListDelegate
                }

                ColumnLayout {
                    Layout.topMargin: 8
                    spacing: 8
                    width: parent.width

                    Rectangle { height: 1; color: "#E8E8E8"; Layout.fillWidth: true }

                    SidebarButton {
                        icon: "🌐"; text: "从网络添加仓库"
                        accent: true
                        onClicked: checkoutDrawer.open()
                    }
                    SidebarButton {
                        icon: "📂"; text: "添加本地仓库"
                        accent: true
                        onClicked: addLocalDrawer.open()
                    }
                    SidebarButton {
                        icon: "📋"; text: "查看同步记录"
                        accent: true
                        onClicked: syncRecordsDrawer.open()
                    }

                    Rectangle { height: 1; color: "#E8E8E8"; Layout.topMargin: 4; Layout.bottomMargin: 4; Layout.fillWidth: true }

                    SidebarButton {
                        icon: "⚙️"; text: "设置"
                        onClicked: settingsDrawer.open()
                    }
                    SidebarButton {
                        icon: "ℹ️"; text: "关于"
                        onClicked: aboutDrawer.open()
                    }
                }
            }
        }

        // ---------- 右侧主区域 ----------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#FFFFFF"

            ColumnLayout {
                anchors.fill: parent

                // 路径栏
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    color: "#FAFBFC"
                    border.color: "#E8E8E8"
                    border.width: 0

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12

                        Label {
                            text: "路径:"
                            font.pixelSize: 12
                            color: "#666666"
                        }
                        Label {
                            id: pathText
                            text: configService.localPath()
                            font.pixelSize: 13
                            color: "#333333"
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                        }

                        Button {
                            id: refreshBtn
                            text: "刷新"
                            implicitWidth: 70
                            implicitHeight: 32
                            onClicked: fileModel.load(currentPath)
                        }
                    }
                }

                // 文件列表
                ListView {
                    id: fileListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    cacheBuffer: 200
                    model: fileModel
                    delegate: fileItemDelegate
                    visible: fileModel.count > 0

                    // 表头
                    Rectangle {
                        width: parent.width
                        height: 32
                        color: "#F8F9FA"
                        border.color: "#EEEEEE"
                        z: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 12
                            spacing: 0

                            Label { Layout.minimumWidth: 40; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "#" }
                            Label { Layout.minimumWidth: 288; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "名称" }
                            Label { Layout.minimumWidth: 70; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "大小" }
                            Label { Layout.minimumWidth: 90; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "修改时间" }
                            Label { Layout.minimumWidth: 140; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; text: "SVN 状态" }
                        }
                    }
                }

                // 空状态
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Label {
                        text: "暂无文件，请在左侧添加仓库"
                        font.pixelSize: 14
                        font.italic: true
                        color: "#999999"
                        anchors.centerIn: parent
                        visible: fileModel.count === 0
                    }
                }
            }
        }
    }

    // ================================================================
    // 状态栏
    // ================================================================
    Rectangle {
        id: statusBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 36
        color: "#1E88E5"
        z: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8

            // 仓库名标签
            Label {
                id: statusBarRepoLabel
                text: configService.activeRepositoryName || ""
                font.pixelSize: 11
                font.weight: Font.DemiBold
                color: "#FFFFFF"
                background: Rectangle {
                    color: "#1565C0"
                    radius: 3
                    anchors.fill: parent
                    anchors.margins: 2
                }
                padding: 3
                visible: text !== ""
            }

            Label {
                id: statusBarPathLabel
                text: currentPath || configService.localPath()
                font.pixelSize: 12
                color: "#FFFFFF"
                Layout.fillWidth: true
                elide: Text.ElideMiddle
            }

            // 同步指示器（旋转圆点）
            Rectangle {
                id: syncIndicator
                width: 10
                height: 10
                radius: 5
                color: "#FFFFFF"
                visible: running
                property bool running: false
                SequentialAnimation on color {
                    running: syncIndicator.running
                    loops: Animation.Infinite
                    ColorAnimation { from: "#FFFFFF"; to: "#BBDEFB"; duration: 600 }
                    ColorAnimation { from: "#BBDEFB"; to: "#FFFFFF"; duration: 600 }
                }
            }

            Label {
                id: statusBarText
                text: "就绪"
                font.pixelSize: 12
                color: "#FFFFFF"
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }
    }

    // ================================================================
    // 右键菜单
    // ================================================================
    Menu {
        id: fileContextMenu
        MenuItem { text: "在资源管理器中打开"; onClicked: openInExplorer() }
        MenuSeparator { }
        MenuItem { text: "复制路径"; onClicked: copyPath() }
        MenuItem { text: "复制 SVN URL"; onClicked: copyUrl() }
        MenuSeparator { }
        MenuItem { text: "粘贴"; onClicked: pasteFile() }
        MenuItem { text: "新建文件夹"; onClicked: newFolder() }
        MenuItem { text: "重命名"; onClicked: renameFile() }
        MenuSeparator { }
        MenuItem { text: "SVN 还原 (Revert)"; onClicked: revertFile() }
        MenuItem { text: "SVN 差异对比 (Diff)"; onClicked: diffFile() }
        MenuItem { text: "SVN 添加 (Add)"; onClicked: addFile() }
        MenuItem { text: "SVN 删除 (Delete)"; onClicked: deleteFile() }
        MenuSeparator { }
        MenuItem { text: "刷新"; onClicked: fileModel.load(currentPath) }
        MenuSeparator { }
        MenuItem { text: "手工同步"; onClicked: manualSync() }
    }

    // ================================================================
    // Delegate 定义
    // ================================================================
    Component {
        id: fileItemDelegate
        FileListItem {
            id: fileListItem
            name: model.name
            fullPath: model.fullPath
            isDirectory: model.isDirectory
            svnStatus: model.svnStatus
            fileSizeDisplay: model.fileSizeDisplay
            lastModifiedDisplay: model.lastModifiedDisplay
            isCurrentPath: model.isCurrentPath

            onDoubleClicked: {
                if (model.isCurrentPath) {
                    goUp()
                } else if (model.isDirectory) {
                    navigateInto(model.fullPath)
                } else {
                    openInExplorer()
                }
            }

            onContextMenuRequested: {
                fileContextMenu.currentIndex = index
                var g = fileListItem.mapToItem(null, x, y)
                fileContextMenu.popup(g.x, g.y)
            }
        }
    }

    Component {
        id: repoListDelegate
        RepoListItem {
            repoName: model.name
            repoPath: model.path
            repoType: model.type
            isSelected: model.isSelected
            onItemClicked: selectRepo(index)
            onRemoveClicked: removeRepo(index)
        }
    }

    // ================================================================
    // 辅助函数
    // ================================================================
    property string currentPath: configService.localPath()

    // pathText 跟随 currentPath 变化
    onCurrentPathChanged: {
        pathText.text = currentPath
        statusBarPathLabel.text = currentPath
    }

    function navigateInto(path) {
        currentPath = path
        pathText.text = path
        statusBarPathLabel.text = path
        fileModel.load(path)
        syncEngine.watchPath(path)
    }

    function goUp() {
        var parentPath = currentPath.substring(0, currentPath.lastIndexOf("/"))
        if (parentPath === "") return
        navigateInto(parentPath)
    }

    function openInExplorer() {
        Qt.openUrlExternally("file:" + currentPath)
    }

    function copyPath() {
        Clipboard.text = fileModel.getFilePath(fileContextMenu.currentIndex)
    }

    function deleteFile() {
        var idx = fileContextMenu.currentIndex
        if (idx < 0) return
        var filePath = fileModel.getFilePath(idx)
        var fileName = filePath.split("/").pop()
        confirmDeleteDialog.fileToDelete = filePath
        confirmDeleteDialog.fileName = fileName
        confirmDeleteDialog.open()
    }

    function pasteFile() {
        var pastedPath = fileModel.pasteFromClipboard()
        if (pastedPath !== "") {
            svnClient.add(pastedPath)
            svnClient.commit(pastedPath, "[SVNFileBox] Paste: " + pastedPath.split("/").pop())
            fileModel.load(currentPath)
        }
    }

    function newFolder() {
        newFolderDialog.open()
    }

    function renameFile() {
        var idx = fileContextMenu.currentIndex
        if (idx < 0) return
        renameDialog.oldPath = fileModel.getFilePath(idx)
        renameDialog.oldName = renameDialog.oldPath.split("/").pop()
        renameDialog.newNameField.text = renameDialog.oldName
        renameDialog.open()
    }

    function manualSync() { syncEngine.syncNow() }

    function revertFile() {
        var idx = fileContextMenu.currentIndex
        if (idx < 0) return
        var filePath = fileModel.getFilePath(idx)
        if (svnClient.revert(filePath)) {
            fileModel.load(currentPath)
            statusBarText.text = "已还原: " + filePath.split("/").pop()
        }
    }

    function diffFile() {
        var idx = fileContextMenu.currentIndex
        if (idx < 0) return
        var filePath = fileModel.getFilePath(idx)
        var result = svnClient.diff(filePath)
        if (result.exitCode === 0) {
            statusBarText.text = "差异已生成: " + filePath.split("/").pop()
        }
    }

    function addFile() {
        var idx = fileContextMenu.currentIndex
        if (idx < 0) return
        var filePath = fileModel.getFilePath(idx)
        if (svnClient.add(filePath)) {
            fileModel.load(currentPath)
            statusBarText.text = "已添加: " + filePath.split("/").pop()
        }
    }

    function copyUrl() {
        var idx = fileContextMenu.currentIndex
        if (idx < 0) return
        var filePath = fileModel.getFilePath(idx)
        var info = svnClient.info(filePath)
        if (info && info.url) {
            Clipboard.text = info.url
            statusBarText.text = "已复制 SVN URL"
        }
    }

    function selectRepo(index) {
        for (var i = 0; i < repoListModel.count; i++) {
            repoListModel.setProperty(i, "isSelected", i === index)
        }
        var repo = repoListModel.get(index)
        configService.setActiveRepository(repo.name)
        statusBarRepoLabel.text = repo.name
        navigateInto(repo.path)
    }

    function removeRepo(index) {
        var repo = repoListModel.get(index)
        configService.removeRepository(repo.name)
        repoListModel.remove(index)
    }

    // ================================================================
    // Drawer：设置
    // ================================================================
    Drawer {
        id: settingsDrawer
        edge: Qt.RightEdge
        width: parent.width * 0.5
        height: parent.height - 36
        y: 0

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
                        onClicked: saveSettings()
                    }
                    Button {
                        text: "关闭"
                        implicitWidth: 80; implicitHeight: 36
                        onClicked: settingsDrawer.close()
                    }
                }

                Label { id: settingsStatusLabel; text: ""; color: "#4CAF50"; font.pixelSize: 12 }
            }
        }
    }

    function saveSettings() {
        configService.syncIntervalMinutes = parseInt(syncIntervalInput.text) || 1
        configService.proxyUrl = proxyUrlInput.text
        configService.syncRecordRetentionDays = parseInt(retentionDaysInput.text) || 30
        configService.autoStart = autoStartCheck.checked
        configService.minimizeToTray = minimizeToTrayCheck.checked
        configService.saveConfig()
        settingsStatusLabel.text = "设置已保存"
    }

    // ================================================================
    // Drawer：从网络添加仓库
    // ================================================================
    Drawer {
        id: checkoutDrawer
        edge: Qt.RightEdge
        width: parent.width * 0.5
        height: parent.height - 36
        y: 0

        Rectangle {
            anchors.fill: parent
            color: "#FFFFFF"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 40
                spacing: 16

                Label {
                    text: "从网络添加仓库"
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    color: "#1A1A2E"
                }

                GridLayout {
                    columns: 2
                    rowSpacing: 12
                    columnSpacing: 12

                    Label { text: "仓库名称:"; Layout.alignment: Qt.AlignRight }
                    TextField {
                        id: checkoutNameInput
                        placeholderText: "例如：我的项目"
                        Layout.minimumWidth: 300
                    }

                    Label { text: "SVN 仓库 URL:"; Layout.alignment: Qt.AlignRight }
                    TextField {
                        id: checkoutUrlInput
                        placeholderText: "https://example.com/svn/repo"
                        Layout.minimumWidth: 300
                    }

                    Label { text: "用户名:"; Layout.alignment: Qt.AlignRight }
                    TextField {
                        id: checkoutUserInput
                        placeholderText: "（可选）"
                        Layout.minimumWidth: 300
                    }

                    Label { text: "密码:"; Layout.alignment: Qt.AlignRight }
                    TextField {
                        id: checkoutPassInput
                        echoMode: TextInput.Password
                        placeholderText: "（可选）"
                        Layout.minimumWidth: 300
                    }
                }

                RowLayout {
                    spacing: 12
                    Button {
                        text: "确认"
                        implicitWidth: 100; implicitHeight: 36
                        onClicked: doCheckout()
                    }
                    Button {
                        text: "取消"
                        implicitWidth: 80; implicitHeight: 36
                        onClicked: checkoutDrawer.close()
                    }
                }

                Item { Layout.fillHeight: true }
                Label { id: checkoutStatusLabel; text: ""; color: "#E53935"; font.pixelSize: 12 }
            }
        }
    }

    function doCheckout() {
        var name = checkoutNameInput.text.trim()
        var url = checkoutUrlInput.text.trim()
        var user = checkoutUserInput.text.trim()
        var pass = checkoutPassInput.text
        if (!name || !url) {
            checkoutStatusLabel.text = "请填写仓库名称和 URL"
            return
        }
        checkoutStatusLabel.text = "正在检出..."
        var result = svnClient.checkout(url, configService.localPath() + "/" + name, user, pass)
        if (result.exitCode === 0) {
            var localPath = configService.localPath() + "/" + name
            configService.addRepository({ name: name, url: url, localPath: localPath, username: user, password: pass })
            syncEngine.startSync(name, localPath, url, user, pass)
            checkoutDrawer.close()
        } else {
            checkoutStatusLabel.text = "检出失败：" + result.error
        }
    }

    // ================================================================
    // Drawer：添加本地仓库
    // ================================================================
    Drawer {
        id: addLocalDrawer
        edge: Qt.RightEdge
        width: parent.width * 0.5
        height: parent.height - 36
        y: 0

        Rectangle {
            anchors.fill: parent
            color: "#FFFFFF"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 40
                spacing: 16

                Label {
                    text: "添加本地仓库"
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    color: "#1A1A2E"
                }

                Label {
                    text: "选择一个已有的 SVN 工作副本目录"
                    font.pixelSize: 13
                    color: "#666666"
                }

                RowLayout {
                    spacing: 12
                    TextField {
                        id: localRepoPathInput
                        placeholderText: "选择本地 SVN 工作副本目录"
                        Layout.fillWidth: true
                        readOnly: true
                    }
                    Button {
                        text: "浏览..."
                        implicitWidth: 90; implicitHeight: 36
                        onClicked: localRepoDialog.open()
                    }
                }

                FolderDialog {
                    id: localRepoDialog
                    title: "选择 SVN 工作副本目录"
                    folder: shortcuts.home
                    onAccepted: {
                        localRepoPathInput.text = localRepoDialog.folder
                    }
                }

                RowLayout {
                    spacing: 12
                    Button {
                        text: "确认"
                        implicitWidth: 100; implicitHeight: 36
                        onClicked: doAddLocal()
                    }
                    Button {
                        text: "取消"
                        implicitWidth: 80; implicitHeight: 36
                        onClicked: addLocalDrawer.close()
                    }
                }

                Item { Layout.fillHeight: true }
                Label { id: addLocalStatusLabel; text: ""; color: "#E53935"; font.pixelSize: 12 }
            }
        }
    }

    function doAddLocal() {
        var path = localRepoPathInput.text.trim()
        if (!path) { addLocalStatusLabel.text = "请先选择目录"; return }
        // FolderDialog.folder 返回 file:///... 格式，转为普通路径
        if (path.startsWith("file:///")) path = path.substring(8)
        if (path.startsWith("file:")) path = path.substring(5)
        if (!svnClient.isValidWorkingCopy(path)) {
            addLocalStatusLabel.text = "这不是一个有效的 SVN 工作副本"
            return
        }
        var info = svnClient.info(path)
        var name = path.substring(path.lastIndexOf("/") + 1)
        configService.addRepository({ name: name, url: info.url, localPath: path, username: "", password: "" })
        syncEngine.startSync(name, path, info.url, "", "")
        addLocalDrawer.close()
    }

    // ================================================================
    // Drawer：同步记录
    // ================================================================
    Drawer {
        id: syncRecordsDrawer
        edge: Qt.RightEdge
        width: parent.width * 0.6
        height: parent.height - 36
        y: 0

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
                        onClicked: syncRecordService.clearRecords()
                    }
                    Button {
                        text: "关闭"
                        implicitWidth: 60; implicitHeight: 30
                        onClicked: syncRecordsDrawer.close()
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

    // ================================================================
    // Drawer：关于
    // ================================================================
    Drawer {
        id: aboutDrawer
        edge: Qt.RightEdge
        width: parent.width * 0.5
        height: parent.height - 36
        y: 0

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
                    onClicked: aboutDrawer.close()
                }
            }
        }
    }

    // ================================================================
    // Dialog：新建文件夹
    // ================================================================
    QtQuick.Dialogs.Dialog {
        id: newFolderDialog
        title: "新建文件夹"
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        parent: mainWindow
        width: 400

        ColumnLayout {
            spacing: 12
            Label { text: "文件夹名称：" }
            TextField {
                id: newFolderNameField
                Layout.fillWidth: true
                placeholderText: "请输入文件夹名称"
                focus: true
                onAccepted: newFolderDialog.accept()
            }
        }

        onAccepted: {
            var name = newFolderNameField.text
            if (!name || name.trim() === "") return
            var targetPath = currentPath + "/" + name.trim()
            var dir = fileModel.createDirectory(targetPath)
            if (dir) {
                svnClient.mkdir(targetPath)
                svnClient.commit(targetPath, "[SVNFileBox] Add folder: " + name.trim())
                fileModel.load(currentPath)
            }
            newFolderNameField.text = ""
        }
    }

    // ================================================================
    // Dialog：确认删除
    // ================================================================
    QtQuick.Dialogs.Dialog {
        id: confirmDeleteDialog
        title: "确认删除"
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        parent: mainWindow
        width: 400

        property string fileToDelete: ""
        property string fileName: ""

        Label {
            text: "确定要删除 " + confirmDeleteDialog.fileName + " 吗？"
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            if (svnClient.remove(fileToDelete)) {
                svnClient.commit(fileToDelete, "[SVNFileBox] Delete: " + fileToDelete.split("/").pop())
                fileModel.load(currentPath)
            }
        }
    }

    // ================================================================
    // Dialog：重命名
    // ================================================================
    QtQuick.Dialogs.Dialog {
        id: renameDialog
        title: "重命名"
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        parent: mainWindow
        width: 400

        property string oldPath: ""
        property string oldName: ""

        ColumnLayout {
            spacing: 12
            Label { text: "新名称：" }
            TextField {
                id: newNameField
                Layout.fillWidth: true
                placeholderText: "请输入新名称"
                focus: true
                onAccepted: renameDialog.accept()
            }
        }

        onOpened: {
            newNameField.text = oldName
        }

        onAccepted: {
            var newName = newNameField.text.trim()
            if (newName === "" || newName === oldName) return
            var newPath = oldPath.substring(0, oldPath.lastIndexOf("/") + 1) + newName
            if (svnClient.move(oldPath, newPath)) {
                svnClient.commit(newPath, "[SVNFileBox] Rename: " + oldName + " → " + newName)
                fileModel.load(currentPath)
            }
        }
    }

    // ================================================================
    // Dialog：SVN 冲突解决
    // ================================================================
    QtQuick.Dialogs.Dialog {
        id: conflictDialog
        title: "SVN 冲突"
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        parent: mainWindow
        width: 500

        property var conflictFileList: []

        onVisibleChanged: {
            if (visible && conflictFileList.length > 0) {
                conflictFileLabel.text = conflictFileList.join("\n")
            }
        }

        ColumnLayout {
            spacing: 12
            Label {
                text: "检测到 " + conflictDialog.conflictFileList.length + " 个文件存在冲突："
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: "#E53935"
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: conflictList.height + 16
                color: "#FFF3E0"
                border.color: "#FFB74D"
                border.width: 1
                radius: 4

                ListView {
                    id: conflictList
                    anchors.fill: parent
                    anchors.margins: 8
                    model: conflictDialog.conflictFileList
                    interactive: false
                    clip: true
                    delegate: Label {
                        text: modelData
                        font.pixelSize: 12
                        color: "#E53935"
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Label {
                text: "请选择保留哪个版本："
                font.pixelSize: 12
                color: "#666666"
            }

            RowLayout {
                spacing: 12
                Rectangle {
                    Layout.minimumWidth: 130
                    implicitHeight: 36
                    color: "#1E88E5"
                    radius: 4
                    Button {
                        anchors.fill: parent
                        text: "保留我的版本"
                        palette.buttonText: "white"
                        flat: true
                        onClicked: {
                            for (var i = 0; i < conflictDialog.conflictFileList.length; i++) {
                                var filePath = conflictDialog.conflictFileList[i]
                                syncEngine.resolveConflictForFile(filePath, "mine-conflict")
                            }
                            conflictDialog.close()
                            fileModel.load(currentPath)
                        }
                    }
                }
                Rectangle {
                    Layout.minimumWidth: 130
                    implicitHeight: 36
                    color: "#E53935"
                    radius: 4
                    Button {
                        anchors.fill: parent
                        text: "使用服务器版本"
                        palette.buttonText: "white"
                        flat: true
                        onClicked: {
                            for (var i = 0; i < conflictDialog.conflictFileList.length; i++) {
                                var filePath = conflictDialog.conflictFileList[i]
                                syncEngine.resolveConflictForFile(filePath, "theirs-conflict")
                            }
                            conflictDialog.close()
                            fileModel.load(currentPath)
                        }
                    }
                }
            }
        }

        onClosed: {
            conflictDialog.conflictFileList = []
        }
    }
}

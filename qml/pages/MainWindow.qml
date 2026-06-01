import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
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
    // globalManager 信号连接（rename/url-edit 持久化已由 C++ 端完成；
    // 这里仅同步 UI 状态，避免整个 sidebar 重建）
    // ================================================================
    Connections {
        target: globalManager
        function onFilesChanged() {
            fileModel.load(currentPath)
        }
        function onRepositoryChanged(oldName, newName, oldUrl, newUrl) {
            // In-place update of the sidebar ListModel row
            for (var i = 0; i < repoListModel.count; i++) {
                var row = repoListModel.get(i)
                if (row.name === oldName) {
                    var patched = {
                        name: newName,
                        path: row.path,
                        url: newUrl,
                        username: row.username,
                        password: row.password,
                        type: row.type,
                        isSelected: row.isSelected
                    }
                    repoListModel.setProperty(i, "name", newName)
                    repoListModel.setProperty(i, "url", newUrl)
                    if (statusBarRepoLabel.text === oldName) {
                        statusBarRepoLabel.text = newName
                    }
                    if (row.isSelected) {
                        currentPath = row.path
                        pathText.text = row.path
                    }
                    break
                }
            }
            statusBarText.text = "已更新仓库: " + newName
        }
    }

    // ================================================================
    // fileModel.copyProgress / copyCompleted
    // ================================================================
    Connections {
        target: fileModel
        function onCopyProgress(currentIndex, totalCount, bytesCopied, totalBytes, currentFile) {
            copyProgressDialog.currentIndex = currentIndex
            copyProgressDialog.totalCount = totalCount
            copyProgressDialog.bytesCopied = bytesCopied
            copyProgressDialog.totalBytes = totalBytes
            copyProgressDialog.currentFile = currentFile
        }
        function onCopyCompleted(copiedCount, skippedCount, overwrittenCount, errorMessage) {
            copyProgressDialog.close()
            if (errorMessage) {
                statusBarText.text = "导入失败: " + errorMessage
            } else {
                statusBarText.text = "已导入 " + copiedCount + " 项，跳过 " + skippedCount + " 项，覆盖 " + overwrittenCount + " 项"
                // Trigger svn add (fileModel has already done it async)
                // Just reload the list to show new files
                fileModel.load(currentPath)
            }
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
        anchors.bottomMargin: 36
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
                            text: configService.localPath
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

                // 文件列表（StackLayout：DropArea 底 + ListView 顶）
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: 0

                    // DropArea 底层：接收拖放
                    DropArea {
                        id: fileDropArea
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Rectangle {
                            anchors.fill: parent
                            color: "#E3F2FD"
                            border.color: "#1E88E5"
                            border.width: 2
                            radius: 4
                            visible: parent.containsDrag

                            Label {
                                text: "释放文件以导入"
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                                color: "#1E88E5"
                                anchors.centerIn: parent
                            }
                        }

                        onDropped: {
                            var urls = dropEvent.urls
                            if (urls.length === 0) return
                            var dest = currentPath
                            // Strip "file:///" prefix for local paths
                            var paths = []
                            for (var i = 0; i < urls.length; i++) {
                                var url = urls[i]
                                if (url.startsWith("file:///")) url = url.substring(8)
                                paths.push(url)
                            }
                            // Reset progress dialog and open
                            copyProgressDialog.wasCancelled = false
                            copyProgressDialog.currentIndex = 0
                            copyProgressDialog.totalCount = 0
                            copyProgressDialog.bytesCopied = 0
                            copyProgressDialog.totalBytes = 0
                            copyProgressDialog.currentFile = ""
                            copyProgressDialog.open()
                            fileModel.importFilesAsync(paths, dest)
                        }
                    }

                    // ListView 顶层
                    ListView {
                        id: fileListView
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

            Label {
                id: statusBarRepoLabel
                text: configService.activeRepositoryName() || ""
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
                text: currentPath || configService.localPath
                font.pixelSize: 12
                color: "#FFFFFF"
                Layout.fillWidth: true
                elide: Text.ElideMiddle
            }

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
        property int currentIndex: -1
        MenuItem { text: "在资源管理器中打开"; onTriggered: openInExplorer() }
        MenuSeparator { }
        MenuItem { text: "复制路径"; onTriggered: copyPath() }
        MenuItem { text: "复制 SVN URL"; onTriggered: copyUrl() }
        MenuSeparator { }
        MenuItem { text: "粘贴"; onTriggered: pasteFile() }
        MenuItem { text: "新建文件夹"; onTriggered: newFolder() }
        Menu {
            title: "新建文件"
            MenuItem { text: "文本文档 (.txt)";   onTriggered: newFile("txt") }
            MenuItem { text: "Word 文档 (.docx)";  onTriggered: newFile("docx") }
            MenuItem { text: "Excel 工作表 (.xlsx)"; onTriggered: newFile("xlsx") }
            MenuItem { text: "PPT 演示文稿 (.pptx)"; onTriggered: newFile("pptx") }
            MenuItem { text: "PNG 图片 (.png)";    onTriggered: newFile("png") }
            MenuItem { text: "BMP 图片 (.bmp)";    onTriggered: newFile("bmp") }
        }
        MenuItem { text: "重命名"; onTriggered: renameFile() }
        MenuSeparator { }
        MenuItem { text: "SVN 还原 (Revert)"; onTriggered: revertFile() }
        MenuItem { text: "SVN 差异对比 (Diff)"; onTriggered: diffFile() }
        MenuItem { text: "SVN 添加 (Add)"; onTriggered: addFile() }
        MenuItem { text: "SVN 删除 (Delete)"; onTriggered: deleteFile() }
        MenuSeparator { }
        MenuItem { text: "刷新"; onTriggered: fileModel.load(currentPath) }
        MenuSeparator { }
        MenuItem { text: "手工同步"; onTriggered: manualSync() }
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
            onRenameClicked: {
                renameRepoDialog.index = index
                renameRepoDialog.oldName = model.name
                renameRepoDialog.newNameField.text = model.name
                renameRepoDialog.open()
            }
            onEditUrlClicked: {
                editRepoDialog.index = index
                editRepoDialog.name = model.name
                editRepoDialog.urlField.text = model.url
                editRepoDialog.open()
            }
        }
    }

    // ================================================================
    // 辅助函数
    // ================================================================
    property string currentPath: configService.localPath

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
            globalManager.activeManager.activeExecutor().executeLocalWrite(4, pastedPath)
            globalManager.activeManager.activeExecutor().executeHeavyWrite(2, pastedPath, "", "[SVNFileBox] Paste: " + pastedPath.split("/").pop())
        }
    }

    function newFolder() {
        newFolderDialog.open()
    }

    function newFile(ext) {
        var baseName = {
            "txt": "新建文本文档",
            "docx": "新建 Microsoft Word 文档",
            "xlsx": "新建 Microsoft Excel 工作表",
            "pptx": "新建 Microsoft PowerPoint 演示文稿",
            "png": "新建 PNG 图片",
            "bmp": "新建 BMP 图片"
        }[ext] || "新建文件"

        newFileNameInput.text = baseName
        newFileDialog.ext = ext
        newFileDialog.open()
    }

    function renameFile() {
        var idx = fileContextMenu.currentIndex
        if (idx < 0) return
        renameDialog.oldPath = fileModel.getFilePath(idx)
        renameDialog.oldName = renameDialog.oldPath.split("/").pop()
        renameDialog.newNameField.text = renameDialog.oldName
        renameDialog.open()
    }

    function manualSync() {
        globalManager.activeManager.syncEngine.syncNow()
    }

    function revertFile() {
        var idx = fileContextMenu.currentIndex
        if (idx < 0) return
        var filePath = fileModel.getFilePath(idx)
        globalManager.activeManager.activeExecutor().executeLocalWrite(15, filePath)
        statusBarText.text = "已还原: " + filePath.split("/").pop()
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
        globalManager.activeManager.activeExecutor().executeLocalWrite(4, filePath)
        statusBarText.text = "已添加: " + filePath.split("/").pop()
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

    function saveSettings() {
        var content = settingsDrawer.contentItem
        if (!content) return
        configService.syncIntervalMinutes = parseInt(content.syncIntervalValue) || 1
        configService.proxyUrl = content.proxyUrlText
        configService.autoStart = content.autoStartChecked
        configService.autoStartMinimize = content.autoStartMinimizeChecked
        configService.minimizeToTray = content.minimizeToTrayChecked
        configService.fileTransferTimeoutSeconds = parseInt(content.fileTransferTimeoutValue) || 120
        // Theme: 0=system, 1=light, 2=dark
        configService.theme = content.themeIndex === 1 ? "light" : (content.themeIndex === 2 ? "dark" : "system")
        // Language: 0=auto, 1=zh, 2=en
        configService.language = content.languageIndex === 1 ? "zh" : (content.languageIndex === 2 ? "en" : "auto")
        configService.saveConfig()
        content.statusText = "设置已保存"
    }

    function doCheckout() {
        var content = checkoutDrawer.contentItem
        if (!content) return
        var name = content.nameText.trim()
        var url = content.urlText.trim()
        var user = content.userText.trim()
        var pass = content.passText
        if (!name || !url) {
            content.statusText = "请填写仓库名称和 URL"
            return
        }

        // Check duplicate by URL
        for (var i = 0; i < repoListModel.count; i++) {
            if (repoListModel.get(i).url === url) {
                content.statusText = "该网络仓库地址已存在，不能重复添加"
                return
            }
        }

        var localPath = configService.localPath + "/" + name
        if (File.exists(localPath)) {
            content.statusText = "本地路径已存在，请换一个仓库名称"
            return
        }

        // Create parent directory
        var parentDir = localPath.substring(0, localPath.lastIndexOf("/"))
        if (!File.exists(parentDir)) {
            fileModel.createDirectory(parentDir)
        }

        content.statusText = "正在检出..."
        var result = svnClient.checkout(url, localPath, user, pass)
        if (result.exitCode === 0) {
            configService.addRepository({ name: name, url: url, localPath: localPath, username: user, password: pass })
            var newRepo = { name: name, path: localPath, url: url, username: user, password: pass, type: "Remote", isSelected: false }
            repoListModel.append(newRepo)
            selectRepo(repoListModel.count - 1)
            checkoutDrawer.close()
        } else {
            content.statusText = "检出失败：" + result.error
            // Clean up on failure
            if (File.exists(localPath)) {
                svnClient.removeDirectory(localPath)
            }
        }
    }

    function doAddLocal() {
        var path = localRepoPathInput.text.trim()
        if (!path) { addLocalStatusLabel.text = "请先选择目录"; return }
        if (path.startsWith("file:///")) path = path.substring(8)
        if (path.startsWith("file:")) path = path.substring(5)

        // Check duplicate by local path
        for (var i = 0; i < repoListModel.count; i++) {
            if (repoListModel.get(i).path === path) {
                addLocalStatusLabel.text = "本地路径已存在，不能重复添加"
                return
            }
        }

        if (!svnClient.isValidWorkingCopy(path)) {
            addLocalStatusLabel.text = "这不是一个有效的 SVN 工作副本"
            return
        }
        var info = svnClient.info(path)
        var name = path.substring(path.lastIndexOf("/") + 1)
        configService.addRepository({ name: name, url: info.url, localPath: path, username: "", password: "" })
        var newRepo = { name: name, path: path, url: info.url, username: "", password: "", type: "Local", isSelected: false }
        repoListModel.append(newRepo)
        selectRepo(repoListModel.count - 1)
        addLocalDrawer.close()
    }

    // ================================================================
    // Drawer：设置（Loader + sourceComponent）
    // ================================================================
    Drawer {
        id: settingsDrawer
        edge: Qt.RightEdge
        width: parent.width * 0.5
        height: parent.height - 36
        y: 0
        position: 0

        Loader {
            anchors.fill: parent
            sourceComponent: settingsDrawerContent
        }
    }

    Component {
        id: settingsDrawerContent
        SettingsDrawerContent {
            onSaveClicked: saveSettings()
            onCloseClicked: settingsDrawer.close()
        }
    }

    // ================================================================
    // Drawer：从网络添加仓库（Loader）
    // ================================================================
    Drawer {
        id: checkoutDrawer
        edge: Qt.RightEdge
        width: parent.width * 0.5
        height: parent.height - 36
        y: 0
        position: 0

        Loader {
            anchors.fill: parent
            sourceComponent: checkoutDrawerContent
        }
    }

    Component {
        id: checkoutDrawerContent
        CheckoutDrawerContent {
            onConfirmClicked: doCheckout()
            onCancelClicked: checkoutDrawer.close()
        }
    }

    // ================================================================
    // Drawer：添加本地仓库（内联，保持不变）
    // ================================================================
    Drawer {
        id: addLocalDrawer
        edge: Qt.RightEdge
        width: parent.width * 0.5
        height: parent.height - 36
        y: 0
        position: 0

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

    FileDialog {
        id: localRepoDialog
        title: "选择 SVN 工作副本目录"
        fileMode: FileDialog.Directory
        folder: "file:///home/osuser"
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

    // ================================================================
    // Drawer：同步记录（Loader）
    // ================================================================
    Drawer {
        id: syncRecordsDrawer
        edge: Qt.RightEdge
        width: parent.width * 0.6
        height: parent.height - 36
        y: 0
        position: 0

        Loader {
            anchors.fill: parent
            sourceComponent: syncRecordsDrawerContent
        }
    }

    Component {
        id: syncRecordsDrawerContent
        SyncRecordsDrawerContent {
            onClearClicked: syncRecordService.clearRecords()
            onCloseClicked: syncRecordsDrawer.close()
        }
    }

    // ================================================================
    // Drawer：关于（Loader）
    // ================================================================
    Drawer {
        id: aboutDrawer
        edge: Qt.RightEdge
        width: parent.width * 0.5
        height: parent.height - 36
        y: 0
        position: 0

        Loader {
            anchors.fill: parent
            sourceComponent: aboutDrawerContent
        }
    }

    Component {
        id: aboutDrawerContent
        AboutDrawerContent {
            onCloseClicked: aboutDrawer.close()
        }
    }

    // ================================================================
    // confirmDeleteDialog
    Popup {
        id: confirmDeleteDialog
        property string fileToDelete: ""
        property string fileName: ""
        anchors.centerIn: parent
        width: 360
        height: confirmDeleteDialogContent.height + 40
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 20
        Column {
            id: confirmDeleteDialogContent
            spacing: 16
            Label {
                text: "确认删除"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Label {
                text: "确定要删除 \"" + confirmDeleteDialog.fileName + "\" 吗？此操作不可撤销。"
                wrapMode: Text.WordWrap
                width: 300
            }
            Row {
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter
                Button {
                    text: "取消"
                    onClicked: confirmDeleteDialog.close()
                }
                Button {
                    text: "确定"
                    highlighted: true
                    onClicked: {
                        confirmDeleteDialog.close()
                        globalManager.activeManager.activeExecutor().executeLocalWrite(6, confirmDeleteDialog.fileToDelete)
                    }
                }
            }
        }
    }

    // newFolderDialog
    Popup {
        id: newFolderDialog
        anchors.centerIn: parent
        width: 360
        height: newFolderDialogContent.height + 40
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 20
        property alias newNameField: newFolderNameInput
        Column {
            id: newFolderDialogContent
            spacing: 16
            Label {
                text: "新建文件夹"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            TextField {
                id: newFolderNameInput
                placeholderText: "新文件夹"
                width: 300
            }
            Row {
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter
                Button {
                    text: "取消"
                    onClicked: newFolderDialog.close()
                }
                Button {
                    text: "确定"
                    highlighted: true
                    onClicked: {
                        if (newFolderNameInput.text.trim() !== "") {
                            var newPath = currentPath + "/" + newFolderNameInput.text.trim()
                            if (fileModel.createDirectory(newPath)) {
                                globalManager.activeManager.activeExecutor().executeLocalWrite(8, newPath)
                            }
                        }
                        newFolderDialog.close()
                    }
                }
            }
        }
    }

    // newFileDialog
    Popup {
        id: newFileDialog
        anchors.centerIn: parent
        width: 360
        height: newFileDialogContent.height + 40
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 20
        property string ext: "txt"
        property alias newNameField: newFileNameInput
        Column {
            id: newFileDialogContent
            spacing: 16
            Label {
                text: "新建文件"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            TextField {
                id: newFileNameInput
                placeholderText: "新建文件"
                width: 300
            }
            Label {
                text: "类型: ." + newFileDialog.ext
                color: "#666"
            }
            Row {
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter
                Button {
                    text: "取消"
                    onClicked: newFileDialog.close()
                }
                Button {
                    text: "确定"
                    highlighted: true
                    onClicked: {
                        var name = newFileNameInput.text.trim()
                        if (name === "") { newFileDialog.close(); return }
                        if (!name.endsWith("." + newFileDialog.ext))
                            name += "." + newFileDialog.ext
                        var fullPath = currentPath + "/" + name
                        if (fileModel.createFile(fullPath)) {
                            globalManager.activeManager.activeExecutor().executeLocalWrite(4, fullPath)
                        }
                        newFileDialog.close()
                    }
                }
            }
        }
    }

    // renameDialog
    Popup {
        id: renameDialog
        anchors.centerIn: parent
        width: 360
        height: renameDialogContent.height + 40
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 20
        property string oldPath: ""
        property string oldName: ""
        property alias newNameField: renameNameInput
        Column {
            id: renameDialogContent
            spacing: 16
            Label {
                text: "重命名"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            TextField {
                id: renameNameInput
                placeholderText: "新名称"
                width: 300
            }
            Row {
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter
                Button {
                    text: "取消"
                    onClicked: renameDialog.close()
                }
                Button {
                    text: "确定"
                    highlighted: true
                    onClicked: {
                        if (renameNameInput.text.trim() !== "" && renameNameInput.text !== renameDialog.oldName) {
                            var newPath = currentPath + "/" + renameNameInput.text.trim()
                            globalManager.activeManager.activeExecutor().executeLocalWrite(5, newPath, renameDialog.oldPath)
                        }
                        renameDialog.close()
                    }
                }
            }
        }
    }

    // renameRepoDialog — rename a repository (P3)
    Popup {
        id: renameRepoDialog
        anchors.centerIn: parent
        width: 380
        height: renameRepoDialogContent.height + 40
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 20
        property int index: -1
        property string oldName: ""
        property string statusText: ""
        property alias newNameField: renameRepoNameInput
        Column {
            id: renameRepoDialogContent
            spacing: 12
            width: 340
            Label {
                text: "重命名仓库"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Label {
                text: "原名称：" + renameRepoDialog.oldName
                font.pixelSize: 12
                color: "#666666"
            }
            TextField {
                id: renameRepoNameInput
                placeholderText: "新名称"
                width: 340
                selectByMouse: true
            }
            Label {
                text: renameRepoDialog.statusText
                color: "#E53935"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                width: 340
                visible: text !== ""
            }
            Row {
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter
                Button {
                    text: "取消"
                    onClicked: renameRepoDialog.close()
                }
                Button {
                    text: "确定"
                    highlighted: true
                    onClicked: {
                        var newName = renameRepoNameInput.text.trim()
                        if (newName === "" || newName === renameRepoDialog.oldName) {
                            renameRepoDialog.statusText = "新名称无效或与原名称相同"
                            return
                        }
                        // Duplicate check against other rows
                        for (var i = 0; i < repoListModel.count; i++) {
                            if (i !== renameRepoDialog.index && repoListModel.get(i).name === newName) {
                                renameRepoDialog.statusText = "已存在同名仓库"
                                return
                            }
                        }
                        // Find the manager. We use the helper which iterates
                        // managers(); rename triggers repositoryChanged which
                        // updates the ListModel + config in-place.
                        globalManager.renameRepoByName(renameRepoDialog.oldName, newName)
                        renameRepoDialog.close()
                    }
                }
            }
        }
    }

    // editRepoDialog — change a repository's URL (P3, mirrors WPF EditRepoWindow)
    Popup {
        id: editRepoDialog
        anchors.centerIn: parent
        width: 420
        height: editRepoDialogContent.height + 40
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 20
        property int index: -1
        property string name: ""
        property string statusText: ""
        property alias urlField: editRepoUrlInput
        Column {
            id: editRepoDialogContent
            spacing: 12
            width: 380
            Label {
                text: "修改仓库 URL"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Label {
                text: "仓库：" + editRepoDialog.name
                font.pixelSize: 12
                color: "#666666"
            }
            TextField {
                id: editRepoUrlInput
                placeholderText: "新 URL (例如 https://svn.example.com/repo)"
                width: 380
                selectByMouse: true
            }
            Label {
                text: "注：仅修改本地记录的 URL；如需重新定位工作副本，请使用 \"svn switch --relocate\"。"
                font.pixelSize: 10
                color: "#999999"
                wrapMode: Text.WordWrap
                width: 380
            }
            Label {
                text: editRepoDialog.statusText
                color: "#E53935"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                width: 380
                visible: text !== ""
            }
            Row {
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter
                Button {
                    text: "取消"
                    onClicked: editRepoDialog.close()
                }
                Button {
                    text: "保存"
                    highlighted: true
                    onClicked: {
                        var newUrl = editRepoUrlInput.text.trim()
                        if (newUrl === "") {
                            editRepoDialog.statusText = "URL 不能为空"
                            return
                        }
                        // Resolve the manager by name on the C++ side and
                        // persist + emit repositoryChanged. QML never holds
                        // a raw RepoManager* pointer.
                        globalManager.updateRepoUrlByName(editRepoDialog.name, newUrl)
                        editRepoDialog.close()
                    }
                }
            }
        }
    }

    // copyProgressDialog — shows async file copy progress
    Popup {
        id: copyProgressDialog
        anchors.centerIn: parent
        width: 400
        height: copyProgressColumn.height + 40
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 20
        property int totalCount: 0
        property int currentIndex: 0
        property real bytesCopied: 0
        property real totalBytes: 0
        property string currentFile: ""
        property bool wasCancelled: false

        Column {
            id: copyProgressColumn
            spacing: 12
            width: 360

            Label {
                text: "正在导入文件"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            Label {
                text: copyProgressDialog.currentIndex + " / " + copyProgressDialog.totalCount
                color: "#666"
            }

            ProgressBar {
                id: copyProgressBar
                width: 360
                from: 0
                to: copyProgressDialog.totalBytes > 0 ? copyProgressDialog.totalBytes : 1
                value: copyProgressDialog.bytesCopied
            }

            Label {
                text: copyProgressDialog.currentFile
                elide: Text.ElideMiddle
                width: 360
                color: "#999"
                font.pixelSize: 12
            }

            Row {
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter
                Button {
                    text: copyProgressDialog.wasCancelled ? "关闭" : "取消"
                    onClicked: {
                        if (!copyProgressDialog.wasCancelled) {
                            fileModel.cancelCopy()
                            copyProgressDialog.wasCancelled = true
                        } else {
                            copyProgressDialog.close()
                        }
                    }
                }
            }
        }
    }

    // conflictDialog
    Popup {
        id: conflictDialog
        anchors.centerIn: parent
        width: 500
        height: 320
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 20
        property var conflictFileList: []
        Column {
            anchors.fill: parent
            spacing: 12
            Label {
                text: "冲突检测"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Label {
                text: "检测到文件冲突，请手动解决："
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }
            ListView {
                id: conflictList
                height: 180
                width: parent.width
                anchors.left: parent.left
                anchors.right: parent.right
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
            Button {
                text: "确定"
                anchors.horizontalCenter: parent.horizontalCenter
                highlighted: true
                onClicked: conflictDialog.close()
            }
        }
        Component.onCompleted: conflictDialog.close()
        Component.onDestruction: conflictDialog.close()
    }
}

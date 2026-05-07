Item {
    id: mainWindow

    property var conflictFileList: []

    // ---------- SyncEngine 信号连接 ----------
    Connections {
        target: syncEngine
        function onSyncNotification(message) {
            statusBarText.text = message
        }
        function onFilesChanged() {
            fileModel.load(currentPath)
        }
        function onConflictDetected(files) {
            conflictFileList = files
            conflictDialog.open()
        }
    }

    // ---------- 主布局：左侧栏 + 右侧内容 ----------
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---------- 左侧边栏 ----------
        Rectangle {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            color: "#FFFFFF"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 0

                // 标题
                Label {
                    text: "仓库列表"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: "#1A1A2E"
                    Layout.bottomMargin: 8
                }

                // 仓库列表
                ListView {
                    id: repoListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: ListModel { id: repoListModel }
                    delegate: repoListDelegate
                }

                // 按钮区
                ColumnLayout {
                    Layout.topMargin: 8
                    spacing: 8
                    width: parent.width

                    Rectangle { height: 1; color: "#E8E8E8"; Layout.fillWidth: true }

                    SidebarButton {
                        icon: "🌐"; text: "从网络添加仓库"
                        accent: true
                        onClicked: contentStack.push(checkoutPageContent)
                    }
                    SidebarButton {
                        icon: "📂"; text: "添加本地仓库"
                        accent: true
                        onClicked: contentStack.push(addLocalPageContent)
                    }
                    SidebarButton {
                        icon: "📋"; text: "查看同步记录"
                        accent: true
                        onClicked: contentStack.push(syncRecordsPageContent)
                    }

                    Rectangle { height: 1; color: "#E8E8E8"; Layout.topMargin: 4; Layout.bottomMargin: 4; Layout.fillWidth: true }

                    SidebarButton {
                        icon: "⚙️"; text: "设置"
                        onClicked: contentStack.push(settingsPageContent)
                    }
                    SidebarButton {
                        icon: "ℹ️"; text: "关于"
                        onClicked: contentStack.push(aboutPageContent)
                    }
                }
            }
        }

        // ---------- 右侧主区域 ----------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

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
                        flat: true
                        id: refreshBtn
                        text: "刷新"
                        implicitWidth: 70
                        implicitHeight: 32
                        onClicked: fileModel.load(currentPath)
                    }
                }
            }

            // ---------- 页面路由 ----------
            StackView {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true

                // 主内容页
                Item {
                    id: rightPanelContent
                    anchors.fill: parent

                    ColumnLayout {
                        anchors.fill: parent

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

                                    Label { Layout.minimumWidth: 40; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888" }
                                    Label { Layout.minimumWidth: 288; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; leftPadding: 8 }
                                    Label { Layout.minimumWidth: 70; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; horizontalAlignment: Text.AlignHCenter }
                                    Label { Layout.minimumWidth: 90; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888"; horizontalAlignment: Text.AlignRight }
                                    Label { Layout.minimumWidth: 140; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888" }
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

        // ============================================================
        // 从网络添加仓库
        // ============================================================
        Item {
            id: checkoutPageContent
            anchors.fill: parent

            Rectangle {
                anchors.fill: parent
                color: "#FFFFFF"
                anchors.margins: 20

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
                            placeholderText: "https://svn.example.com/repos/myproject"
                            Layout.minimumWidth: 300
                        }
                        Label { text: "用户名:"; Layout.alignment: Qt.AlignRight }
                        TextField { id: checkoutUserInput; placeholderText: "（可选）"; Layout.minimumWidth: 300 }
                        Label { text: "密码:"; Layout.alignment: Qt.AlignRight }
                        TextField { id: checkoutPassInput; echoMode: TextInput.Password; placeholderText: "（可选）"; Layout.minimumWidth: 300 }
                    }

                    RowLayout {
                        spacing: 12
                        Button {
                            flat: true
                            text: "确认"
                            implicitWidth: 100; implicitHeight: 36
                            contentItem: Label { text: parent.text; color: "#FFFFFF"; horizontalAlignment: Text.AlignHCenter }
                            onClicked: doCheckout()
                        }
                        Button {
                            flat: true
                            text: "取消"
                            implicitWidth: 100; implicitHeight: 36
                            contentItem: Label { text: parent.text; color: "#1A1A2E"; horizontalAlignment: Text.AlignHCenter }
                            onClicked: contentStack.pop()
                        }
                    }

                    Item { Layout.fillHeight: true }

                    Label { id: checkoutStatusLabel; text: ""; color: "#E53935"; font.pixelSize: 12 }
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
                    configService.addRepository({ name: name, url: url, localPath: localPath, username: user, password: pass, type: "Network" })
                    syncEngine.startSync(name, localPath, url, user, pass)
                    contentStack.pop()
                } else {
                    checkoutStatusLabel.text = "检出失败：" + result.error
                }
            }
        }

        // ============================================================
        // 添加本地仓库
        // ============================================================
        Item {
            id: addLocalPageContent
            anchors.fill: parent

            Rectangle {
                anchors.fill: parent
                color: "#FFFFFF"
                anchors.margins: 20

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
                            flat: true
                            text: "浏览..."
                            implicitWidth: 90; implicitHeight: 36
                            contentItem: Label { text: parent.text; color: "#1A1A2E"; horizontalAlignment: Text.AlignHCenter }
                            onClicked: localRepoDialog.open()
                        }
                    }

                    FileDialog {
                        id: localRepoDialog
                        title: "选择 SVN 工作副本目录"
                        currentFolder: shortcuts.home
                        modality: Qt.WindowModal
                        onAccepted: {
                            localRepoPathInput.text = currentFolder
                        }
                    }

                    Label { id: addLocalStatusLabel; text: ""; color: "#E53935"; font.pixelSize: 12 }

                    RowLayout {
                        spacing: 12
                        Button {
                            flat: true
                            text: "确认"
                            implicitWidth: 100; implicitHeight: 36
                            contentItem: Label { text: parent.text; color: "#FFFFFF"; horizontalAlignment: Text.AlignHCenter }
                            onClicked: doAddLocal()
                        }
                        Button {
                            flat: true
                            text: "取消"
                            implicitWidth: 100; implicitHeight: 36
                            contentItem: Label { text: parent.text; color: "#1A1A2E"; horizontalAlignment: Text.AlignHCenter }
                            onClicked: contentStack.pop()
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            function doAddLocal() {
                var path = localRepoPathInput.text.trim()
                if (!path) { addLocalStatusLabel.text = "请先选择目录"; return }
                if (!svnClient.isValidWorkingCopy(path)) {
                    addLocalStatusLabel.text = "这不是一个有效的 SVN 工作副本"
                    return
                }
                var info = svnClient.info(path)
                var name = path.substring(path.lastIndexOf("/") + 1)
                configService.addRepository({ name: name, url: info.url, localPath: path, username: "", password: "", type: "Local" })
                syncEngine.startSync(name, path, info.url, "", "")
                contentStack.pop()
            }
        }

        // ============================================================
        // 同步记录
        // ============================================================
        Item {
            id: syncRecordsPageContent
            anchors.fill: parent

            Rectangle {
                anchors.fill: parent
                color: "#FFFFFF"
                anchors.margins: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: "同步记录"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            color: "#1A1A2E"
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            flat: true
                            text: "← 返回"
                            implicitWidth: 80; implicitHeight: 30
                            contentItem: Label { text: parent.text; color: "#1A1A2E"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
                            onClicked: contentStack.pop()
                        }
                    }

                    Rectangle { height: 1; color: "#E8E8E8"; Layout.fillWidth: true }

                    // 表头
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        color: "#F8F9FA"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            spacing: 0
                            Label { Layout.minimumWidth: 160; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888" }
                            Label { Layout.minimumWidth: 120; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888" }
                            Label { Layout.minimumWidth: 280; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888" }
                            Label { Layout.minimumWidth: 100; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888" }
                            Label { Layout.minimumWidth: 80; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888" }
                            Label { Layout.fillWidth: true; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#888888" }
                        }
                    }

                    ListView {
                        id: syncRecordListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        cacheBuffer: 200
                        model: ListModel { id: syncRecordListModel }

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
                                Label { Layout.minimumWidth: 80; text: model.result; font.pixelSize: 12; color: model.result === "Success" ? "#00A650" : (model.result === "Failed" ? "#E53935" : "#FB8C00") }
                                Label { Layout.fillWidth: true; text: model.message; font.pixelSize: 12; color: "#888888"; elide: Text.ElideRight }
                            }
                        }
                    }
                }
            }
        }

        // ============================================================
        // 设置
        // ============================================================
        Item {
            id: settingsPageContent
            anchors.fill: parent

            Rectangle {
                anchors.fill: parent
                color: "#FFFFFF"
                anchors.margins: 20

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
                            flat: true
                            text: "保存"
                            implicitWidth: 100; implicitHeight: 36
                            contentItem: Label { text: parent.text; color: "#FFFFFF"; horizontalAlignment: Text.AlignHCenter }
                            onClicked: saveSettings()
                        }
                        Button {
                            flat: true
                            text: "← 返回"
                            implicitWidth: 80; implicitHeight: 36
                            contentItem: Label { text: parent.text; color: "#1A1A2E"; horizontalAlignment: Text.AlignHCenter }
                            onClicked: contentStack.pop()
                        }
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
            }
        }

        // ============================================================
        // 关于
        // ============================================================
        Item {
            id: aboutPageContent
            anchors.fill: parent

            Rectangle {
                anchors.fill: parent
                color: "#FFFFFF"
                anchors.margins: 20

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
                        flat: true
                        text: "← 返回"
                        implicitWidth: 80; implicitHeight: 36
                        contentItem: Label { text: parent.text; color: "#1A1A2E"; horizontalAlignment: Text.AlignHCenter }
                        onClicked: contentStack.pop()
                    }
                }
            }
        }

            // ---------- 底栏状态栏 ----------
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                color: "#1E88E5"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12

                    Label {
                        text: pathText.text
                        font.pixelSize: 12
                        color: "#FFFFFF"
                        Layout.fillWidth: true
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
        }
    }

            // ---------- 右键菜单 ----------
            Menu {
                id: fileContextMenu
                MenuItem { text: "在资源管理器中打开"; onClicked: openInExplorer() }
                MenuSeparator { }
                MenuItem { text: "复制路径"; onClicked: copyPath() }
                MenuSeparator { }
                MenuItem { text: "粘贴"; onClicked: pasteFile() }
                MenuItem { text: "新建文件夹"; onClicked: newFolder() }
                MenuItem { text: "重命名"; onClicked: renameFile() }
                MenuSeparator { }
                MenuItem { text: "删除"; onClicked: deleteFile() }
                MenuSeparator { }
                MenuItem { text: "刷新"; onClicked: fileModel.load(currentPath) }
                MenuSeparator { }
                MenuItem { text: "手工同步"; onClicked: manualSync() }
            }
                    // ---------- 文件列表项代理 ----------
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
                        fileContextMenu.popup()
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
            // ---------- 内部状态 ----------
            property string currentPath: configService.localPath()

            // ---------- 内部方法 ----------
            function navigateInto(path) {
                currentPath = path
                pathText.text = path
                fileModel.load(path)
                syncEngine.watchPath(path)
            }

            function goUp() {
                var parentPath = currentPath.substring(0, currentPath.lastIndexOf("/"))
                if (parentPath === "") return
                navigateInto(parentPath)
            }

            function openInExplorer() {
                Qt.openUrlExternally("file://" + fileModel.getFilePath(fileContextMenu.currentIndex))
            }

            function copyPath() {
                Clipboard.text = fileModel.getFilePath(fileContextMenu.currentIndex)
            }

            function deleteFile() {
                var idx = fileContextMenu.currentIndex
                if (idx < 0) return
                var filePath = fileModel.getFilePath(idx)
                var fileName = fileModel.getFilePath(idx).split("/").pop()
                confirmDeleteDialog.fileToDelete = filePath
                confirmDeleteDialog.fileName = fileName
                confirmDeleteDialog.open()
            }

            function doDeleteFile(filePath) {
                if (svnClient.remove(filePath)) {
                    svnClient.commit(filePath, "[SVNFileBox] Delete: " + filePath.split("/").pop())
                    fileModel.load(currentPath)
                }
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
                // Show input dialog
                newFolderDialog.open()
            }

            function doCreateFolder(name) {
                if (!name || name.trim() === "") return
                var targetPath = currentPath + "/" + name.trim()
                // Create physical directory
                var dir = fileModel.createDirectory(targetPath)
                if (dir) {
                    // Tell SVN to track it
                    svnClient.mkdir(targetPath)
                    // Commit
                    svnClient.commit(targetPath, "[SVNFileBox] Add folder: " + name.trim())
                    // Refresh file list
                    fileModel.load(currentPath)
                }
            }
            function renameFile() {
                var idx = fileContextMenu.currentIndex
                if (idx < 0) return
                renameDialog.oldPath = fileModel.getFilePath(idx)
                renameDialog.oldName = renameDialog.oldPath.split("/").pop()
                renameDialog.newNameField.text = renameDialog.oldName
                renameDialog.open()
            }

            function doRenameFile(oldPath, newPath, newName) {
                if (newName.trim() === "" || newName === oldPath.split("/").pop()) return
                if (svnClient.move(oldPath, newPath)) {
                    svnClient.commit(newPath, "[SVNFileBox] Rename: " + oldPath.split("/").pop() + " → " + newName.trim())
                    fileModel.load(currentPath)
                }
            }
            function manualSync() { syncEngine.syncNow() }

            function selectRepo(index) {
                for (var i = 0; i < repoListModel.count; i++) {
                    repoListModel.setProperty(i, "isSelected", i === index)
                }
                var repo = repoListModel.get(index)
                configService.setActiveRepository(repo.name)
                navigateInto(repo.path)
            }

            function removeRepo(index) {
                var repo = repoListModel.get(index)
                configService.removeRepository(repo.name)
                repoListModel.remove(index)
            }

    // ---------- Dialogs ----------
    Dialog {
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
                onAccepted: {
                    newFolderDialog.accept()
                }
            }
        }

        onAccepted: {
            if (newFolderNameField.text.trim() !== "") {
                mainWindow.doCreateFolder(newFolderNameField.text)
                newFolderNameField.text = ""
            }
        }
        onRejected: {
            newFolderNameField.text = ""
        }
    }

    // ---------- 确认删除对话框 ----------
    Dialog {
        id: confirmDeleteDialog
        title: "确认删除"
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        parent: mainWindow
        width: 380

        property string fileToDelete: ""
        property string fileName: ""

        ColumnLayout {
            spacing: 12
            Label {
                text: "确定要删除以下文件（夹）吗？\n此操作不可撤销。"
                wrapMode: Text.WordWrap
            }
            Label {
                text: confirmDeleteDialog.fileName
                font.bold: true
                color: "#D32F2F"
                wrapMode: Text.WordWrap
            }
        }

        onAccepted: {
            if (fileToDelete !== "") {
                mainWindow.doDeleteFile(fileToDelete)
                fileToDelete = ""
                fileName = ""
            }
        }
        onRejected: {
            fileToDelete = ""
            fileName = ""
        }
    }

    // ---------- 重命名对话框 ----------
    Dialog {
        id: renameDialog
        title: "重命名"
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        parent: mainWindow
        width: 400

        property string oldPath: ""
        property string oldName: ""
        property alias newNameField: renameNewNameField

        ColumnLayout {
            spacing: 12
            Label { text: "新名称：" }
            TextField {
                id: renameNewNameField
                Layout.fillWidth: true
                placeholderText: "请输入新名称"
                focus: true
            }
        }

        onAccepted: {
            var newName = renameNewNameField.text.trim()
            if (newName === "") {
                renameDialog.close()
                return
            }
            var newPath = currentPath + "/" + newName
            mainWindow.doRenameFile(renameDialog.oldPath, newPath, newName)
            oldPath = ""
            oldName = ""
            renameNewNameField.text = ""
        }
        onRejected: {
            oldPath = ""
            oldName = ""
            renameNewNameField.text = ""
        }
    }

    // ---------- 冲突解决弹窗 ----------
    Dialog {
        id: conflictDialog
        title: "文件冲突"
        standardButtons: Dialog.NoButton
        modal: true
        closePolicy: Popup.NoAutoClose

        ColumnLayout {
            spacing: 12
            width: conflictDialog.availableWidth

            Label {
                text: "以下文件存在冲突，请选择保留哪个版本："
                font.bold: true
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            Repeater {
                model: conflictFileList
                delegate: Label {
                    text: "⚠ " + modelData
                    font.family: "Consolas"
                    font.pixelSize: 13
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#E0E0E0"
            }

            Label {
                text: "⚠ 保留本地：会覆盖服务器上的修改，其他人的更新会丢失"
                color: "#E53935"
                font.pixelSize: 12
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            Label {
                text: "⚠ 使用服务器：会丢弃本地的修改，你未提交的改动会丢失"
                color: "#E53935"
                font.pixelSize: 12
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            RowLayout {
                spacing: 12
                Layout.fillWidth: true

                Button {
                    Layout.preferredWidth: 140
                    Layout.preferredHeight: 40
                    text: "保留本地"
                    background: Rectangle { color: "#1565C0"; radius: 4 }
                    contentItem: Label {
                        text: parent.text
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        syncEngine.resolveConflict("mine-conflict")
                        conflictDialog.close()
                    }
                }

                Button {
                    Layout.preferredWidth: 140
                    Layout.preferredHeight: 40
                    text: "使用服务器"
                    background: Rectangle { color: "#6D4C41"; radius: 4 }
                    contentItem: Label {
                        text: parent.text
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        syncEngine.resolveConflict("theirs-conflict")
                        conflictDialog.close()
                    }
                }
            }
        }
    }
}

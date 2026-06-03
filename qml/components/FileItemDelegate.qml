import QtQuick
import QtQuick.Controls

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

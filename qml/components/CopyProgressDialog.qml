import QtQuick
import QtQuick.Controls

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
            text: qsTr("正在导入文件")
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

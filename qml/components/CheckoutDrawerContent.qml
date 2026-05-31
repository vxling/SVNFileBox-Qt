import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform

Item {
    id: root

    property alias nameText: checkoutNameInput.text
    property alias urlText: checkoutUrlInput.text
    property alias userText: checkoutUserInput.text
    property alias passText: checkoutPassInput.text
    property alias statusText: checkoutStatusLabel.text

    signal confirmClicked()
    signal cancelClicked()

    FileDialog {
        id: checkoutFolderDialog
        title: "选择检出目录"
        folder: "file:///home/osuser"
        onAccepted: {
            checkoutFolderInput.text = checkoutFolderDialog.folder
        }
    }

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
                    onClicked: root.confirmClicked()
                }
                Button {
                    text: "取消"
                    implicitWidth: 80; implicitHeight: 36
                    onClicked: root.cancelClicked()
                }
            }

            Item { Layout.fillHeight: true }
            Label { id: checkoutStatusLabel; text: ""; color: "#E53935"; font.pixelSize: 12 }
        }
    }
}

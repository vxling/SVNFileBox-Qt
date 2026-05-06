import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property string icon: ""
    property string text: ""
    property bool accent: false
    property bool selected: false
    signal clicked()

    radius: 8
    color: {
        if (selected) return "#1E88E5"
        if (mouseArea.containsMouse) return "#1E88E5"
        return accent ? "#E3F2FD" : "#F5F7FA"
    }
    border.color: {
        if (selected) return "#1565C0"
        if (mouseArea.containsMouse) return "#1565C0"
        return accent ? "#BBDEFB" : "#E0E0E0"
    }
    border.width: 1

    implicitHeight: 40
    implicitWidth: parent ? parent.width : 0

    Behavior on color { ColorAnimation { duration: 150 } }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 10

        Label {
            text: root.icon
            font.pixelSize: 16
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            text: root.text
            font.pixelSize: 13
            font.weight: Font.Medium
            color: {
                if (selected) return "#FFFFFF"
                if (mouseArea.containsMouse) return "#FFFFFF"
                return "#1A1A2E"
            }
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }
    }
}

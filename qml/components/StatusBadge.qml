import QtQuick

Rectangle {
    id: root
    property string svnStatus: "Normal"  // Normal Modified Added Deleted Conflicted Unversioned Missing Hidden

    width: 18
    height: 18
    radius: 9
    visible: svnStatus !== "Hidden"

    color: {
        switch (svnStatus) {
            case "Modified":   return "#1E88E5"
            case "Added":      return "#00A650"
            case "Deleted":   return "#E53935"
            case "Conflicted": return "#FB8C00"
            case "Unversioned": return "#9E9E9E"
            case "Missing":   return "#8E24AA"
            case "Normal":    return "#00C853"
            default:          return "#9E9E9E"
        }
    }

    Label {
        anchors.centerIn: parent
        text: {
            switch (svnStatus) {
                case "Modified":    return "M"
                case "Added":       return "A"
                case "Deleted":     return "D"
                case "Conflicted":  return "C"
                case "Unversioned": return "?"
                case "Missing":     return "!"
                case "Normal":      return "✓"
                default:            return ""
            }
        }
        font.pixelSize: 10
        font.bold: true
        color: "#FFFFFF"
        anchors.centerIn: parent
    }
}

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    property string svnStatus: "Normal"  // Normal Modified Added Deleted Conflicted Unversioned Missing Hidden

    width: 22
    height: 22
    radius: 11

    visible: svnStatus !== "Hidden"

    // Color mapping
    property color statusColor: {
        switch (root.svnStatus) {
            case "Normal":      return "#00C853"  // 绿色 - 已同步
            case "Modified":    return "#FF9800"  // 橙色 - 已修改
            case "Added":      return "#2196F3"  // 蓝色 - 新增
            case "Deleted":    return "#F44336"  // 红色 - 已删除
            case "Conflicted":  return "#9C27B0"  // 紫色 - 冲突
            case "Unversioned": return "#9E9E9E"  // 灰色 - 未纳入版本
            case "Missing":     return "#795548"  // 棕色 - 缺失
            default:           return "#9E9E9E"
        }
    }

    // Icon text mapping
    property string iconText: {
        switch (root.svnStatus) {
            case "Normal":      return "✓"   // 绿色勾 - 已同步
            case "Modified":    return "●"   // 实心圆 - 已修改
            case "Added":       return "+"   // 加号 - 新增
            case "Deleted":     return "×"   // 叉号 - 已删除
            case "Conflicted":  return "!"   // 感叹号 - 冲突
            case "Unversioned": return "?"   // 问号 - 未纳入版本
            case "Missing":     return "!"   // 感叹号 - 缺失
            default:           return ""
        }
    }

    // Icon color: white for dark backgrounds, statusColor for light
    property color iconColor: {
        switch (root.svnStatus) {
            case "Normal":      return "#FFFFFF"  // 白字绿底（勾）
            case "Modified":    return "#FFFFFF"  // 白字橙底（圆点）
            case "Added":       return "#FFFFFF"  // 白字蓝底（加号）
            case "Deleted":     return "#FFFFFF"  // 白字红底（叉）
            case "Conflicted":  return "#FFFFFF"  // 白字紫底（感叹号）
            case "Unversioned": return statusColor  // 灰字灰底（问号）
            case "Missing":     return "#FFFFFF"  // 白字棕底（感叹号）
            default:           return "#FFFFFF"
        }
    }

    color: statusColor

    Label {
        anchors.centerIn: parent
        text: iconText
        font.pixelSize: svnStatus === "Normal" ? 12 : 14
        font.bold: svnStatus === "Normal"
        color: iconColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
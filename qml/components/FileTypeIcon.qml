import QtQuick

Label {
    id: root
    property string fileName: ""
    property bool isDirectory: false

    font.pixelSize: 24
    text: {
        if (isDirectory) return "📁"

        var ext = fileName.substring(fileName.lastIndexOf(".") + 1).toLowerCase()
        switch (ext) {
            case "cs":      return "💻"
            case "xlsx":
            case "xls":     return "📊"
            case "docx":
            case "doc":     return "📝"
            case "pptx":
            case "ppt":     return "📽️"
            case "pdf":     return "📕"
            case "txt":
            case "md":
            case "log":     return "📄"
            case "jpg":
            case "jpeg":
            case "png":
            case "gif":
            case "bmp":
            case "svg":
            case "webp":    return "🖼️"
            case "zip":
            case "rar":
            case "7z":
            case "tar":
            case "gz":     return "📦"
            case "json":
            case "xml":
            case "yaml":
            case "yml":
            case "toml":    return "📋"
            case "html":
            case "htm":
            case "css":
            case "js":
            case "ts":
            case "tsx":
            case "jsx":     return "🌐"
            case "cpp":
            case "c":
            case "h":
            case "hpp":
            case "java":
            case "go":
            case "rs":
            case "py":
            case "rb":
            case "php":     return "📄"
            case "mp3":
            case "wav":
            case "flac":
            case "aac":
            case "ogg":     return "🎵"
            case "mp4":
            case "avi":
            case "mkv":
            case "mov":
            case "wmv":     return "🎬"
            case "exe":
            case "msi":
            case "dmg":
            case "deb":
            case "rpm":
            case "appimage": return "⚙️"
            default:        return "📄"
        }
    }
}

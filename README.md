# SVNFileBox-Qt

跨平台 SVN 同步客户端，基于 Qt 6 + QML + CMake 构建。

本地文件变更自动 commit 到 SVN，服务器更新自动 pull 到本地，保持工作副本始终同步。

## 功能

- 📁 **仓库管理** — 添加本地工作副本 / 从 URL checkout
- 🔄 **自动同步** — 文件变化自动 commit；服务器更新自动 update
- 📋 **同步记录** — 查看每次同步的时间、文件和结果
- 🗂️ **文件浏览** — 双击进入目录，顶部路径栏导航
- ⚙️ **设置** — 同步周期、代理、开机启动
- 📱 **系统托盘** — 最小化到托盘，双击恢复
- 👥 **多仓库后台监控** — 支持最多 3 个仓库同时后台监控，LRU 策略

## 下载

> **Draft Release**: https://github.com/vxling/SVNFileBox-Qt/releases

- 🐧 [svnfilebox-linux.tar.gz](https://github.com/vxling/SVNFileBox-Qt/releases/latest) — Linux
- 🍎 [svnfilebox-macos.tar.gz](https://github.com/vxling/SVNFileBox-Qt/releases/latest) — macOS
- 🪟 [svnfilebox-win64.zip](https://github.com/vxling/SVNFileBox-Qt/releases/latest) — Windows

## 构建

### 依赖

- CMake ≥ 3.16
- Qt 6.4+ (Core, Widgets, Quick, Controls, Labs Platform)
- libsvn-dev ≥ 1.14 (Subversion development libraries)

### 编译

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

Linux/macOS 运行：`./SVNFileBox`  
Windows 运行：`./bin/SVNFileBox.exe`

## 技术栈

- Qt 6.5 + QML + CMake
- libsvn 1.14 原生绑定（Pimpl 封装，仅暴露 Qt 类型）
- QFileSystemWatcher 文件监控 + 定时轮询
- JSON 持久化配置
- 机器绑定密码加密（XOR + Base64）
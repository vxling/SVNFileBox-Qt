# Windows 编译环境搭建指南

## 环境要求

- **操作系统**: Windows 10/11 (64-bit)
- **磁盘空间**: 建议预留 10GB+ 用于 Qt + Visual Studio + 依赖

---

## 一、安装 Visual Studio 2022

1. 下载 [Visual Studio 2022 Community](https://visualstudio.microsoft.com/downloads/)
2. 运行安装程序，选择以下**工作负载**：
   - ✅ **使用 C++ 的桌面开发**（必须）
   - ✅ **Windows 11 SDK**（如果需要）
3. 安装完成后，打开 **x64 Native Tools Command Prompt for VS 2022**（后续所有命令在此命令行中执行）

---

## 二、安装 Qt（推荐 Qt 6.4~6.9）

### 方法 A：使用 aqtinstall（推荐，自动快速）

```powershell
# 安装 Python（如果没有的话）
# 下载地址：https://www.python.org/downloads/

# 安装 aqtinstall
pip install aqtinstall

# 创建 Qt 安装目录（以 6.9 为例，6.4~6.9 均支持）
mkdir C:\Qt\6.9.0

# 下载 Qt 6.9.0 MSVC2022 64-bit（推荐，用 VS2022）
aqt install-qt windows desktop 6.9.0 win64_msvc2022_64 -O C:\Qt\6.9.0

# 或下载 Qt 6.7.2 MSVC2019 64-bit
aqt install-qt windows desktop 6.7.2 win64_msvc2019_64 -O C:\Qt\6.7.2
```

### 方法 B：手动下载 Qt Creator（需要注册 Qt 账号）

1. 访问 https://www.qt.io/download-qt-installer
2. 注册/登录 Qt 账号
3. 下载 **Qt Online Installer**
4. 安装时选择对应版本的 MSVC 编译器，包含以下模块：
   - Qt 6.x → Qt Core、Qt Gui、Qt Widgets
   - Qt 6.x → Qt Quick Controls 2
   - Qt 6.x → Qt QML
   - Qt 6.x → Qt SQL

> ⚠️ **推荐方法 A**，手动安装需要账号且速度较慢。

### Qt 版本与 Visual Studio 版本的对应关系

| Qt 版本 | 推荐 Visual Studio | MSVC 工具集 |
|---------|-------------------|-------------|
| Qt 6.4 ~ 6.7 | Visual Studio 2019 | msvc2019_64 |
| Qt 6.8 ~ 6.9 | Visual Studio 2022 | msvc2022_64 |

---

## 三、安装 vcpkg（用于 libsvn、zlib 等依赖库）

```powershell
# 在 C:\vcpkg 目录下克隆
git clone --depth 1 https://github.com/microsoft/vcpkg.git C:\vcpkg

# 运行引导脚本
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# 安装依赖
.\vcpkg install libsvn:x64-windows zlib:x64-windows
```

> 📌 libsvn 依赖链较长（apr、apr-util、serf 等），编译需要约 10~20 分钟，耐心等待。

---

## 四、编译 SVNFileBox-Qt

### 1. 克隆代码

```powershell
# 在 x64 Native Tools Command Prompt 中执行
git clone https://github.com/vxling/SVNFileBox-Qt.git
cd SVNFileBox-Qt
```

### 2. 配置 CMake（Qt 6.9 + vcpkg 混用方式）

```powershell
# 设置 Qt PATH（每次编译前执行）
# 根据你安装的 Qt 版本和 VS 版本调整路径
set PATH=C:\Qt\6.9.0\6.9.0\msvc2022_64\bin;%PATH%

# 创建 build 目录并配置
# 关键：Qt6_DIR 指向你自己安装的 Qt（不让 vcpkg 提供 Qt）
# vcpkg toolchain 只负责 libsvn、zlib 等非 Qt 库
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DQt6_DIR=C:\Qt\6.9.0\6.9.0\msvc2022_64\lib\cmake\Qt6 ^
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```

> ⚠️ **Qt6_DIR 必须指向你自己安装的 Qt 目录**，不要让 vcpkg 的 Qt6 介入，否则会出现版本冲突。

### 3. 编译

```powershell
cmake --build build --parallel --config Release
```

### 4. 安装和打包

```powershell
# 安装到 release 目录
cmake --install build --config Release --prefix release

# 使用 windeployqt 部署 Qt 运行时
C:\Qt\6.9.0\6.9.0\msvc2022_64\bin\windeployqt release\bin\SVNFileBox.exe --qmldir C:\Qt\6.9.0\6.9.0\msvc2022_64\qml --no-translations

# 复制 vcpkg 的依赖 DLL（libsvn、zlib 等）
copy C:\vcpkg\installed\x64-windows\bin\*.dll release\bin\ 2>nul

# 打包（可选）
cd release
powershell Compress-Archive -Path bin -DestinationPath svnfilebox-win64.zip
```

---

## 五、运行

```powershell
cd release\bin
SVNFileBox.exe
```

> ✅ 首次运行如果提示缺少 DLL，把 Qt 的 bin 目录添加到系统 PATH 环境变量。

---

## 常见问题

### Q1: 编译报 "Cannot find Qt6" 或版本不匹配
检查 CMake 缓存：`build\CMakeCache.txt`
- 确保 `Qt6_DIR` 指向你自己安装的 Qt（如 `C:\Qt\6.9.0\6.9.0\msvc2022_64\lib\cmake\Qt6`）
- 确保 PATH 中包含 Qt 的 bin 目录
- **不要**让 vcpkg 提供 Qt6，vcpkg 的 Qt6 和官方 Qt 可能存在 ABI 不兼容

### Q2: 报 zlib.h / libsvn 找不到
确保 vcpkg 安装了对应库：
```powershell
C:\vcpkg\vcpkg install libsvn:x64-windows zlib:x64-windows
```

### Q3: windeployqt 报 "Cannot find qmake"
确保 PATH 中包含 Qt 的 bin 目录：
```powershell
set PATH=C:\Qt\6.9.0\6.9.0\msvc2022_64\bin;%PATH%
```

### Q4: VS 报 "Cannot find vcpkgxxx.cmake"
确保 vcpkg 已正确引导：
```powershell
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

### Q5: libsvn 编译失败
libsvn 依赖链较长，确保先安装：
```powershell
vcpkg install serf:x64-windows apr:x64-windows apr-util:x64-windows
```

---

## 快速检查清单

- [ ] Visual Studio 2022（含 C++ 桌面开发 workload）
- [ ] Qt 6.4~6.9 MSVC2022 64-bit（aqtinstall 安装）
- [ ] vcpkg 安装并执行 `vcpkg install libsvn:x64-windows zlib:x64-windows`
- [ ] x64 Native Tools Command Prompt for VS 2022
- [ ] Git（用于克隆代码）
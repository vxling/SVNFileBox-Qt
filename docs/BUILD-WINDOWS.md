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

## 二、安装 Qt 6.7.2

### 方法 A：使用 aqtinstall（推荐，自动快速）

```powershell
# 安装 Python（如果没有的话）
# 下载地址：https://www.python.org/downloads/

# 安装 aqtinstall
pip install aqtinstall

# 创建 Qt 安装目录
mkdir C:\Qt\6.7.2

# 下载 Qt 6.7.2 MSVC2019 64-bit
aqt install-qt windows desktop 6.7.2 win64_msvc2019_64 -O C:\Qt\6.7.2
```

### 方法 B：手动下载 Qt Creator（需要注册 Qt 账号）

1. 访问 https://www.qt.io/download-qt-installer
2. 注册/登录 Qt 账号
3. 下载 **Qt Online Installer**
4. 安装时选择：
   - Qt 6.7.2 → MSVC2019 64-bit
   - Qt 6.7.2 → Qt Quick Controls 2
   - Qt 6.7.2 → Qt QML

> ⚠️ 建议使用 **方法 A**，手动安装需要账号且速度较慢。

---

## 三、安装 vcpkg（用于 zlib 等依赖库）

```powershell
# 在 C:\vcpkg 目录下克隆
git clone --depth 1 https://github.com/microsoft/vcpkg.git C:\vcpkg

# 运行引导脚本
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# 安装 zlib（libzip 依赖）
.\vcpkg install zlib:x64-windows
```

> 📌 vcpkg 会自动下载并编译 zlib，无需手动操作。

---

## 四、编译 SVNFileBox-Qt

### 1. 克隆代码

```powershell
# 在 x64 Native Tools Command Prompt 中执行
git clone https://github.com/vxling/SVNFileBox-Qt.git
cd SVNFileBox-Qt
```

### 2. 配置 CMake

```powershell
# 设置环境变量（每次编译前都要执行）
set PATH=C:\Qt\6.7.2\6.7.2\msvc2019_64\bin;%PATH%

# 创建 build 目录并配置
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DQt6_DIR=C:\Qt\6.7.2\6.7.2\msvc2019_64\lib\cmake\Qt6 ^
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```

### 3. 编译

```powershell
cmake --build build --parallel --config Release
```

### 4. 安装和打包

```powershell
# 安装到 release 目录
cmake --install build --config Release --prefix release

# 使用 windeployqt 部署 Qt 运行时
C:\Qt\6.7.2\6.7.2\msvc2019_64\bin\windeployqt release\bin\SVNFileBox.exe --qmldir C:\Qt\6.7.2\6.7.2\msvc2019_64\qml --no-translations

# 复制 vcpkg 的 zlib.dll
copy C:\vcpkg\installed\x64-windows\bin\zlib.dll release\bin\

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

> ✅ 首次运行如果提示缺少 DLL，把 `C:\Qt\6.7.2\6.7.2\msvc2019_64\bin` 添加到系统 PATH 环境变量。

---

## 常见问题

### Q1: 编译报 "Cannot find Qt6"
检查是否设置了 PATH 并正确指定了 `Qt6_DIR`。Qt6_DIR 应指向包含 `Qt6Config.cmake` 的目录。

### Q2: 报 zlib.h 找不到
确保 vcpkg 安装了 zlib：
```powershell
C:\vcpkg\vcpkg install zlib:x64-windows
```

### Q3: windeployqt 报 "Cannot find qmake"
确保 PATH 中包含 Qt 的 bin 目录：
```powershell
set PATH=C:\Qt\6.7.2\6.7.2\msvc2019_64\bin;%PATH%
```

### Q4: Visual Studio 卡在 "Configuring..."
检查 CMake 缓存：`build\CMakeCache.txt`，确保 Qt6_DIR 和 vcpkg 路径正确。

---

## 快速检查清单

- [ ] Visual Studio 2022（含 C++ 桌面开发 workload）
- [ ] Qt 6.7.2 MSVC2019 64-bit
- [ ] vcpkg 安装并执行 `vcpkg install zlib:x64-windows`
- [ ] x64 Native Tools Command Prompt for VS 2022
- [ ] Git（用于克隆代码）
# Windows 编译环境搭建指南

## 环境要求

- **操作系统**: Windows 10/11 (64-bit)
- **磁盘空间**: 建议预留 15GB+ 用于 Qt + Visual Studio + 依赖

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

# 安装时需要选择以下模块：
#   Qt 6.9.0 → Qt Core、Qt GUI、Qt Widgets
#   Qt 6.9.0 → Qt Quick Controls 2、Qt QML、Qt SQL
```

### 方法 B：手动下载 Qt Creator（需要注册 Qt 账号）

1. 访问 https://www.qt.io/download-qt-installer
2. 注册/登录 Qt 账号
3. 下载 **Qt Online Installer**
4. 安装时选择对应版本的 MSVC 编译器，包含以下模块：
   - Qt 6.x → Qt Core、Qt GUI、Qt Widgets
   - Qt 6.x → Qt Quick Controls 2、Qt QML、Qt SQL

> ⚠️ **推荐方法 A**，手动安装需要账号且速度较慢。

### Qt 版本与 Visual Studio 版本的对应关系

| Qt 版本 | 推荐 Visual Studio | MSVC 工具集 |
|---------|-------------------|-------------|
| Qt 6.4 ~ 6.7 | Visual Studio 2019 | msvc2019_64 |
| Qt 6.8 ~ 6.9 | Visual Studio 2022 | msvc2022_64 |

---

## 三、安装 vcpkg（用于 libsvn、zlib）

### 3.1 安装 vcpkg

```powershell
# 在 C:\vcpkg 目录下克隆
git clone --depth 1 https://github.com/microsoft/vcpkg.git C:\vcpkg

# 运行引导脚本
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

### 3.2 安装 libsvn（Subversion）依赖

> ⚠️ libsvn 依赖链较长（apr、apr-util、serf 等），编译需要约 15~30 分钟，耐心等待。

```powershell
# 安装 subversion（vcpkg 端口名就是 subversion，不是 libsvn）
C:\vcpkg\vcpkg install subversion:x64-windows

# 安装 zlib（libzip 依赖）
C:\vcpkg\vcpkg install zlib:x64-windows
```

> 📌 `subversion` 端口会拉取 apr、apr-util、serf 等一系列依赖，自动编译好，不需要单独安装。

---

## 四、修改 CMakeLists.txt 支持 Windows

当前 CMakeLists.txt 使用 pkg-config 找 libsvn，这是 Linux 专用的。在 Windows 上需要改用 vcpkg 的 find_package 方式。

**找到 CMakeLists.txt 中的 libsvn 段落，替换为以下内容：**

```cmake
# ============================================================
# SVN libsvn (libsvn_client + APR runtime)
# Windows: use vcpkg find_package; Linux: use pkg-config
# ============================================================
if(WIN32)
    # vcpkg provides subversion port with cmake config files
    find_package(subversion CONFIG REQUIRED)
    # subversion::svn_client is the main target
    set(SVN_CLIENT_LIBRARIES subversion::svn_client subversion::svn_wc subversion::svn_subr subversion::svn_repos subversion::svn_ra subversion::svn_delta)
    set(SVN_CLIENT_INCLUDE_DIRS "")
    message(STATUS "SVN: using vcpkg subversion")
else()
    # Linux: use pkg-config
    set(PKG_CONFIG_PATH_SVN "/usr/lib/x86_64-linux-gnu/pkgconfig" CACHE STRING "")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SVN_CLIENT REQUIRED libsvn_client libsvn_wc libsvn_subr libsvn_repos libsvn_ra libsvn_delta)
    message(STATUS "SVN include dirs: ${SVN_CLIENT_INCLUDE_DIRS}")
    message(STATUS "SVN libraries:    ${SVN_CLIENT_LIBRARIES}")
endif()
```

---

## 五、编译 SVNFileBox-Qt

### 5.1 克隆代码

```powershell
# 在 x64 Native Tools Command Prompt 中执行
git clone https://github.com/vxling/SVNFileBox-Qt.git
cd SVNFileBox-Qt
```

### 5.2 应用 CMakeLists.txt 修改

> 如果你已经克隆了代码，需要先手动修改 `CMakeLists.txt`（参考第四节的替换内容），再进行配置。

### 5.3 配置 CMake（Qt 6.9 + vcpkg 混用方式）

```powershell
# 设置 Qt PATH（每次编译前执行）
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

### 5.4 编译

```powershell
cmake --build build --parallel --config Release
```

### 5.5 安装和打包

```powershell
# 安装到 release 目录
cmake --install build --config Release --prefix release

# 使用 windeployqt 部署 Qt 运行时
C:\Qt\6.9.0\6.9.0\msvc2022_64\bin\windeployqt release\bin\SVNFileBox.exe --qmldir C:\Qt\6.9.0\6.9.0\msvc2022_64\qml --no-translations

# 复制 vcpkg 的依赖 DLL（subversion、zlib 等）
copy C:\vcpkg\installed\x64-windows\bin\*.dll release\bin\ 2>nul

# 打包（可选）
cd release
powershell Compress-Archive -Path bin -DestinationPath svnfilebox-win64.zip
```

---

## 六、运行

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
C:\vcpkg\vcpkg install subversion:x64-windows zlib:x64-windows
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

### Q5: subversion 编译失败（常见）
subversion 依赖链很长，确保先完整安装所有前置依赖：
```powershell
# 清理之前的失败尝试
C:\vcpkg\vcpkg remove subversion
C:\vcpkg\vcpkg integrate remove

# 重新安装（加 verbose 看进度）
C:\vcpkg\vcpkg install subversion:x64-windows --recurse
```

### Q6: 运行时提示找不到 libsvn.dll
确保 vcpkg 的 DLL 已复制到 output 目录：
```powershell
copy C:\vcpkg\installed\x64-windows\bin\libsvn*.dll release\bin\ 2>nul
copy C:\vcpkg\installed\x64-windows\bin\apr*.dll release\bin\ 2>nul
copy C:\vcpkg\installed\x64-windows\bin\serf*.dll release\bin\ 2>nul
```

---

## 快速检查清单

- [ ] Visual Studio 2022（含 C++ 桌面开发 workload）
- [ ] Qt 6.4~6.9 MSVC2022 64-bit（aqtinstall 安装）
- [ ] vcpkg 安装并执行 `vcpkg install subversion:x64-windows zlib:x64-windows`
- [ ] CMakeLists.txt 已按第四节修改（libsvn find_package 方式）
- [ ] x64 Native Tools Command Prompt for VS 2022
- [ ] Git（用于克隆代码）
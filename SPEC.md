# SVNFileBox-Qt - SPEC.md

> SVN 版 Dropbox。基于 Qt 6.5.3 + QML + CMake 重写，参考 WPF 项目 SVNFileBox 的设计。
> 参考项目源码：`/home/osuser/aiworks/projects/repos/SVNFileBox/`

---

## 核心设计决策

### 冲突策略：Last-Write-Wins

- 以**本地文件最后修改时间**为准，强制覆盖 SVN 端版本
- 无合并、无保留多版本，冲突直接解决
- **理由**: 简单粗暴，对用户友好，避免手动解决冲突的复杂度

### 同步粒度：文件级

- 检测到文件变化（Create/Modify/Delete/Rename）立即提交整个文件
- 不做分块、不做增量，文件级简单直接
- **注意**: SVN 本身版本控制是全文件快照，天然契合此策略

### 手工操作

- **暂不提供**任何手工操作入口（手动提交、手动 checkout 等）
- 仅提供**同步记录**让用户查看历史
- **理由**: MVP 阶段先跑通核心流程，手工操作留作后续迭代

---

## 功能列表

### 1. 仓库管理
- 添加 SVN 仓库（URL + 用户名 + 密码）
- 仓库列表持久化存储（JSON）
- 删除仓库（从本地管理列表移除，不动 SVN 端数据）
- 切换仓库 → 切换工作副本 + 启动/停止监控

### 2. 同步核心
- 监控指定本地文件夹（QFileSystemWatcher）
- 检测文件变化事件：Created / Modified / Deleted / Renamed
- 检测到变化后，延迟一小段时间（如 500ms）合并同批次变化，然后自动 commit 到 SVN
- Last-Write-Wins 冲突解决：取本地文件时间戳 vs SVN 最后版本时间戳，以最新的为准强制覆盖
- 同步完成后记录同步事件
- **全量扫描保护**：每30分钟扫描一次 svn status，将所有 unversioned 和 missing 项统一 commit

### 3. 同步记录查看
- 记录每次同步操作：时间戳、文件路径、操作类型（Add/Modify/Delete/Rename）、结果
- 提供 UI 列表展示同步历史
- 支持滚动加载

### 4. 文件浏览器
- 导航：双击文件夹进入，顶部路径栏显示当前路径
- 返回上级目录行：列表首行显示"返回上级目录"，仓库根目录则不显示
- 刷新按钮：重新加载当前目录

### 5. 文件操作（右键菜单）
- 在文件资源管理器中打开
- 复制路径
- 粘贴文件（Ctrl+V）
- 新建文件夹
- 重命名
- 删除
- 拖拽文件到列表
- 刷新
- 手工同步

### 6. 设置页面
- 同步周期（下行轮询间隔，默认 1 分钟）
- 代理设置
- 开机启动
- 语言

---

## UI 设计

### 主窗口布局

```
┌─────────────────────────────────────────────────────────┐
│  SVNFileBox                              [工具栏按钮]     │
├────────────┬────────────────────────────────────────────┤
│ Repository │ Path: /path/to/workspace            [刷新]    │
│ ─────────  ├────────────────────────────────────────────┤
│ 📁 repo1   │ ← 文件列表 或 同步记录列表（按需切换）          │
│ 📁 repo2   │                                            │
│ ─────────  │                                            │
│ 按钮区:    │                                            │
│ + 从网络添加│                                            │
│ + 添加本地  │                                            │
│ + 查看同步记录│                                          │
│ + 设置      │                                            │
│ + 关于      │                                            │
├────────────┴────────────────────────────────────────────┤
│ 状态栏: 当前路径 + 操作状态                              │
└─────────────────────────────────────────────────────────┘
```

### 窗口尺寸
- 默认：1100 × 580
- 最小：900 × 550
- 居中启动

### 配色方案（参考 WPF）
| 用途 | 颜色 |
|------|------|
| 主色 | `#1E88E5`（蓝） |
| 背景 | `#F0F4F8` |
| 侧边栏背景 | 白色 |
| 侧边栏按钮默认 | `#F5F7FA` |
| 侧边栏按钮悬停/选中 | `#1E88E5` |
| 路径栏背景 | `#FAFBFC` |
| 状态栏背景 | `#1E88E5` |
| 按钮圆角 | 6px（路径栏按钮）/ 8px（侧边栏按钮） |

### 左侧边栏（220px）

**上半部分**：仓库列表
- 卡片式列表项，左侧 3px 蓝色左边框表示选中
- 仓库图标：本地仓库 📂（橙色），网络仓库 🌐（蓝色）
- 每个列表项右上角 X 删除按钮
- 仓库名（粗体）+ 本地路径（灰色小字，超长截断）

**下半部分**：按钮区（垂直排列）
- 🌐 从网络添加仓库（Checkout）
- 📂 添加本地仓库
- 📋 查看同步记录
- ⚙️ 设置
- ℹ️ 关于

### 文件列表列

| 列 | 宽度 | 内容 |
|----|------|------|
| 类型 | 40px | 文件夹图标或按扩展名显示图标（emoji） |
| 名称 | 280px | 文件名，首行可显示"返回上级目录" |
| 状态 | 70px | 彩色圆形徽章，圆角9px，18×18 |
| 大小 | 90px | 文件大小（文件夹留空），自动 B/KB/MB/GB |
| 修改时间 | 140px | `yyyy-MM-dd HH:mm` 格式 |

### 状态徽章颜色（参考 WPF SvnStatusToColorConverter）

| 状态 | 颜色 | 文字 |
|------|------|------|
| Modified | `#1E88E5` 蓝 | M |
| Added | `#00A650` 绿 | A |
| Deleted | `#E53935` 红 | D |
| Conflicted | `#FB8C00` 橙 | C |
| Unversioned | `#9E9E9E` 灰 | ? |
| Missing | `#8E24AA` 紫 | ! |
| Normal（已同步） | `#00C853` 绿 | ✓ |
| Hidden（上级目录行） | 透明 | 无徽章 |

### 文件类型图标（emoji）

| 扩展名 | emoji |
|--------|-------|
| 文件夹 | 📁 |
| .cs | 💻 |
| .xlsx / .xls | 📊 |
| .docx / .doc | 📝 |
| .pptx / .ppt | 📽️ |
| .pdf | 📕 |
| .txt | 📄 |
| .jpg / .png / .gif | 🖼️ |
| .zip / .rar / .7z | 📦 |
| .json / .xml / .yaml | 📋 |
| .html / .css / .js | 🌐 |
| 其他 | 📄 |

### 右键菜单

```
在资源管理器中打开
─────────────
复制路径
─────────────
粘贴
新建文件夹
重命名
─────────────
删除
─────────────
刷新
─────────────
手工同步
```

### 路径栏

- 白色背景，底边框 `#E8E8E8`
- 左侧文字"路径:" + 当前路径
- 右侧"刷新"按钮（卡片样式，悬停蓝色）

### 状态栏

- 蓝底白字（`#1E88E5` 背景）
- 左侧：当前路径
- 右侧：操作状态文字（超长截断）

---

## 数据模型

### FileItem（QML ListView 数据模型）

```cpp
struct FileItem {
    QString name;           // 文件名
    QString fullPath;       // 完整路径
    bool isDirectory;       // 是否文件夹
    qint64 fileSize;        // 文件大小（字节）
    QDateTime lastModified; // 最后修改时间
    SvnStatus svnStatus;    // SVN 状态
};

// 显示用转换属性（QML 直接计算）
QString fileSizeDisplay;   // "1.2 MB" 或 ""（文件夹）
QString lastModifiedDisplay; // "2026-05-06 19:30" 或 ""
QString typeDisplay;       // emoji 图标
```

### Repository（仓库数据模型）

```cpp
struct Repository {
    QString name;           // 显示名称
    QString url;            // SVN 仓库 URL
    QString localPath;      // 本地工作副本路径
    QString username;       // SVN 用户名（可选）
    QString password;       // SVN 密码（DPAPI 加密存储，可选）
    bool isActive;          // 是否当前激活
    RepositoryType type;   // Local / Network
};
```

### SvnStatus（枚举）

```cpp
enum SvnStatus {
    Normal,       // 已同步，无变更
    Modified,     // M - 已修改
    Added,        // A - 新增
    Deleted,      // D - 已删除
    Conflicted,    // C - 冲突
    Unversioned,   // ? - 未纳入版本
    Missing,       // ! - 缺失（被删但 SVN 未知）
    Replaced,      // R - 替换
    Obstructed,   // ~ - 阻塞
    External,      // X - 外部定义
    Unknown,       // I - 未知
    Hidden         // 仅用于"返回上级目录"行，不显示徽章
};
```

### SyncRecord（同步记录）

```cpp
struct SyncRecord {
    QString id;             // UUID
    QDateTime timestamp;    // 时间戳
    QString repoName;       // 仓库名
    QString filePath;       // 文件路径
    QString operation;       // Add / Update / Delete / Update / FullScan / ConflictResolved
    QString result;          // Success / Failed / Skipped
    QString message;         // 详情/错误信息
};
```

### SyncRecordDisplay（同步记录显示模型）

从 SyncRecord 转换而来，供 QML 直接绑定：

```cpp
struct SyncRecordDisplay {
    QString timestampDisplay;  // "2026-05-06 19:30:00"
    QString repoName;
    QString filePath;
    QString operationDisplay; // 带 emoji："🟢 Add" / "🔵 Update" / "🔴 Delete" 等
    QString resultDisplay;    // 带 emoji："✅ Success" / "❌ Failed" / "⏭️ Skipped"
    QString message;
};
```

### AppConfig（应用配置）

```cpp
struct AppConfig {
    bool autoSyncEnabled = true;
    int syncIntervalMinutes = 1;        // 下行轮询间隔（分钟）
    QString conflictStrategy = "LastWriteWins";
    QString proxyUrl;
    int syncRecordRetentionDays = 30;
    bool autoStart = true;              // 开机启动
    bool minimizeToTray = true;          // 最小化到托盘
    QString language = "auto";           // 语言
    QString? activeRepositoryName;
    QList<Repository> repositories;
};
```

---

## 服务层架构

### 1. ConfigService
- **职责**：配置读写（JSON），仓库列表管理，密码加密
- **配置路径**：`~/.svnfilebox/config.json`
- **密码存储**：DPAPI 加密（Windows），Keychain（macOS），libsecret（Linux）
- **API**：
  - `loadConfig()` → `AppConfig`
  - `saveConfig(AppConfig)`
  - `addRepository(Repository)`
  - `removeRepository(QString name)`

### 2. SVNClient（SvnService）
- **职责**：封装所有 svn CLI 调用
- **实现**：QProcess 异步执行 svn 命令，stdout/stderr 捕获
- **SVN 路径查找**：
  - Linux/macOS：`svn`（PATH 中）
  - Windows：依次查找 `TortoiseSVN/bin/svn.exe`、`VisualSVN Server/bin/svn.exe`、`SlikSvn/bin/svn.exe`、`svn`（PATH）
- **API**：
  - `getStatus(QString path)` → `QMap<QString, SvnStatus>`（当前目录状态，非递归）
  - `addFile(QString path)` → bool
  - `deleteFile(QString path)` → bool
  - `commit(QString path, QString message, QString user, QString pass)` → bool
  - `update(QString path)` → bool
  - `info(QString path)` → `SvnInfo`（URL、Revision 等）
  - `checkout(QString url, QString localPath, QString user, QString pass)` → `(output, exitCode, error)`
  - `getWorkingCopyRevision(QString path)` → int（本地 .svn 版本号）
  - `getHeadRevision(QString url, QString user, QString pass)` → int（服务器 HEAD 版本号）
  - `isValidWorkingCopy(QString path)` → bool（检查 .svn/entries 是否存在）
  - `runCommand(QString args, int timeoutMs = 60000)` → `(output, exitCode, error)`

**所有 SVN 命令统一使用 `--xml` 输出格式**，通过 QXmlStreamReader 解析，拒绝文本正则匹配。

**SVN Status 解析（XML）**：
```
svn status --xml --non-interactive <path>
```
解析方式：遍历 `<entry>` 节点，读取 `path` 属性作为文件路径，`item` 属性映射到 SvnStatus：
| `item` 属性值 | SvnStatus |
|--------------|-----------|
| `modified` | Modified |
| `added` | Added |
| `deleted` | Deleted |
| `conflicted` | Conflicted |
| `unversioned` | Unversioned |
| `missing` | Missing |
| `replaced` | Replaced |
| `obstructed` | Obstructed |
| `external` | External |
| `incomplete` | Unknown |
| 无 `item` 属性 | Normal |

特殊情况：
- 当前目录未纳入版本时（`svn status` 无输出或全部是 `?`），主动枚举子文件，全部标记为 Unversioned
- `wc-status` 的 `tree-conflicted` 属性存在时 → Conflicted

**Commit 命令（XML）**：
```
svn commit --xml --non-interactive -m "<message>" "<path>" --username "<user>" --password "<pass>"
```
解析：`<commit>` 节点的 `revision` 属性 → 提交后的版本号

**Update 命令（XML）**：
```
svn update --xml --non-interactive "<path>" --username "<user>" --password "<pass>"
```
解析：遍历 `<update>` 的 `<entry>` 节点获取各文件更新结果；`updated` 节点 `revision` 属性 → 更新后版本号

**Checkout 命令（XML）**：
```
svn checkout --xml --non-interactive "<url>" "<localPath>" --username "<user>" --password "<pass>"
```
解析：遍历 `<checkout>` 的 `<entry>` 节点；`revision` 属性 → checkout 版本号

**Info 命令（XML）**：
```
svn info --xml --non-interactive "<path>"
```
解析：`<entry>` 节点 `revision` 属性 → 当前版本；`<url>` 节点文本 → 仓库 URL

**List 命令（XML）**：
```
svn list --xml --non-interactive "<url>" --username "<user>" --password "<pass>"
```
解析：遍历 `<list>/<entry>` 节点，`<name>` 文本 → 文件名；`kind="file"|"dir"` → 是否文件夹；`<size>` → 文件大小；`<commit>` 的 `revision` 属性 → 版本号；`<date>` 文本 → 修改时间

**Delete / Add / Revert / Resolved 命令（XML）**：
所有写操作命令统一加 `--xml`，解析 `<success>` 属性（`true`/`false`）判断是否成功，`<err>` 节点获取错误信息

**统一错误处理策略**：
- 每个命令执行后检查 exitCode，非 0 则解析 `<err>` 或 stderr
- SVN 认证失败（`E170001`）→ 提示用户名/密码错误
- 工作副本冲突（`E155015`）→ 触发冲突处理流程
- 网络超时 → 重试 3 次，间隔 2/4/8 秒指数退避

### 3. SyncEngine（SyncService）
- **职责**：上行同步（FileWatcher + 防抖）+ 下行同步（轮询）
- **上行**：QFileSystemWatcher 监控 → 5秒防抖 → svn add/commit
- **下行**：定时器轮询（默认60秒）→ svn update → 检测冲突
- **全量扫描**：每30分钟扫描 svn status，补充 FileWatcher 遗漏
- **API**：
  - `startSync(Repository repo)`
  - `stopSync()`
  - `syncNow()`（手工同步：先上行 pending 文件，再下行 poll）
  - `status()` → QString（当前状态描述）
- **信号**：
  - `statusChanged(QString status)`
  - `filesChanged()`

**上行同步流程（FileWatcher + 5秒防抖）**：
```
文件变化事件（Created/Changed/Deleted/Renamed）
    ↓
QFileSystemWatcher 捕获
    ↓
加入 _changedFiles Set，停止防抖计时器，重新启动5秒防抖计时器
    ↓
防抖计时器触发（5秒内无新变化）
    ↓
遍历 _changedFiles，逐个处理：
  ├── 文件不存在 → svn delete（先 svn info 确认是否被跟踪）
  │                   └── svn delete → svn commit → 记录 Delete
  ├── 文件存在，且未被 SVN 管理 → svn add
  └── 文件存在，且已被 SVN 管理 → svn commit
    ↓
清空 _changedFiles
触发 filesChanged 信号（刷新文件列表）
```

**删除文件特殊处理**：
- FileWatcher 触发删除时，文件已不存在
- 先执行 `svn info <filePath>` 确认是否被 SVN 跟踪
- 未跟踪则跳过（从未 add 过的文件不需要 delete）
- 已跟踪则 `svn delete` → `svn commit`

**重命名处理**：
- QFileSystemWatcher.Renamed 提供 `oldPath` 和 `newPath`
- 两个路径都加入 _changedFiles
- 处理时旧路径走 delete 流程，新路径走 add 流程

**下行同步流程（轮询）**：
```
定时器触发（默认60秒）
    ↓
_isPolling 互斥锁（防止并发轮询）
    ↓
重试 pending 文件（之前被锁定的文件）
    ↓
获取本地版本：svn info → Revision
获取服务器版本：svn info -r HEAD <url> → Revision
    ↓
服务器版本 <= 本地版本？
    └── 是 → 返回，无事发生
    ↓
服务器版本 > 本地版本 → svn update
    ↓
svn update 成功？
    ├── 成功 → 记录 Update 成功，触发 filesChanged
    └── 失败（locked/conflict/error）
              ├── locked → 提取锁定文件，加入 pending，重试队列
              │              记录 Skipped，通知用户"文件被占用，已跳过"
              ├── conflict → HandleConflictsAsync()
              │              记录 ConflictResolved
              └── 其他错误 → 记录 Failed，通知用户
```

**全量扫描流程（每30分钟）**：
```
定时器触发（30分钟）
    ↓
svn status <repoPath>（全量，非单目录）
    ↓
遍历状态：
  ├── Unversioned → svn add --force
  └── Missing → svn delete
    ↓
若有变更 → svn commit → 记录 FullScan
```

**冲突处理（Last-Write-Wins）**：
```
svn update 发现冲突（status 输出有 'C' 开头行）
    ↓
遍历冲突文件，获取冲突列表
    ↓
每个冲突文件：
  获取本地文件最后修改时间（QFileInfo.lastModified()）
  获取 SVN 服务器版本最后修改时间（svn log -r HEAD）
    ↓
本地修改时间 >= 服务器修改时间？
    ├── 是 → 保留本地，执行 svn resolved 标记冲突解决
    └── 否 → svn update 合并，再以本地为准 svn commit，svn resolved
    ↓
记录 ConflictResolved
```

### 4. FileWatcherService
- **职责**：封装 QFileSystemWatcher，提供变化文件列表收集 + 防抖
- **NotifyFilter**：`FileName | DirectoryName | LastWrite | Size`
- **IncludeSubdirectories**：true（递归监控）
- **防抖**：收集 5 秒内的变化事件，合并为一批后触发
- **重试机制**：FileWatcher 报错时（如目录被删），5秒后尝试重连，最长10秒间隔
- **过滤**：排除 `.svn` 目录内的变化
- **API**：
  - `startWatching(QString path)`
  - `stopWatching()`
  - `setDebounceMs(int ms)`
- **信号**：
  - `filesChanged(QStringList files)`

### 5. SyncRecordService
- **职责**：同步记录持久化（JSON 文件）
- **记录路径**：`~/.svnfilebox/sync_records/<repoName>.json`（按仓库分文件）
- **保留策略**：按 `syncRecordRetentionDays` 自动清理
- **加载**：启动时从 `sync_records/` 目录加载所有 `.json` 文件
- **API**：
  - `addRecord(QString repoName, QString filePath, QString operation, QString result, QString message = "")`
  - `getRecords(QString repoName)` → `QList<SyncRecord>`
  - `setRetentionDays(int days)`
- **操作类型显示（带 emoji）**：
  - Add → "🟢 Add"
  - Update → "🔵 Update"
  - Delete → "🔴 Delete"
  - Rename → "🟡 Rename"
  - ConflictResolved → "🟠 Conflict"
  - FullScan → "⚙️ FullScan"
- **结果类型显示（带 emoji）**：
  - Success → "✅ Success"
  - Failed → "❌ Failed"
  - Skipped → "⏭️ Skipped"

---

## 数据流

### 流程一：上行同步（本地 → SVN）

```
文件变化（Created/Changed/Deleted/Renamed）
    ↓
QFileSystemWatcher 捕获事件
    ↓
收集到变化队列，5秒防抖计时开始
    ↓
5秒内无新变化，防抖计时器触发
    ↓
遍历变化文件，逐个判断：
  ├── 新文件 → svn add
  ├── 已删除 → svn delete（先 info 确认是否被跟踪）
  └── 修改 → 对比本地时间戳 vs SVN 版本时间戳
            └── 若本地更新 → svn commit
    ↓
commit 完成后刷新文件列表视图
    ↓
写入同步记录（operation=Add/Update/Delete）
```

### 流程二：下行同步（SVN → 本地）

```
定时器触发（默认60秒）
    ↓
获取本地工作副本当前版本号（svn info）
获取服务器 HEAD 版本号（svn info -r HEAD URL）
    ↓
服务器版本 == 本地版本？
    ├── 是 → 无事发生，等待下次轮询
    └── 否 → 执行 svn update
              ↓
         svn update 遇见锁定文件（如 Excel 打开中）？
           ├── 无锁定 → 正常更新，刷新文件列表
           └── 有锁定 → 跳过被占用文件，加入 pendingUpdates 队列
                        继续更新其他文件
                        通知用户：「file.xlsx 被占用，已跳过」
              ↓
         svn update 遇见冲突？
           ├── 无冲突 → 更新成功，刷新文件列表
           └── 有冲突 → 按 Last-Write-Wins 处理：
                        若本地时间戳 > 服务器 → 保留本地，svn resolved
                        若服务器更新 → svn update 合并后本地覆盖，svn resolved
              ↓
         写入同步记录（operation=Update/ConflictResolved）
    ↓
下次轮询 → 先重试 pendingUpdates 队列中的文件
           → 再检查新变化
           → 连续失败3次 → 标记「同步失败」，提示用户手动处理
```

### 流程三：添加仓库

#### 从网络添加（Checkout）
```
用户点击「从网络添加仓库」
    ↓
弹出 CheckoutWindow，输入：
  - 仓库名称（必填，用户自定义）
  - SVN 仓库 URL（必填，格式验证）
  - 用户名（可选）
  - 密码（可选）
  - 本地路径自动生成：~/.svnfilebox/workcopies/<仓库名称>
    ↓
点击「确认」
    ↓
执行 svn checkout URL localPath --username user --password pass
    ↓
保存仓库到配置列表
    ↓
启动 QFileSystemWatcher 监控该仓库
启动下行轮询定时器
    ↓
加载文件列表视图
```

#### 添加本地仓库
```
用户点击「添加本地仓库」
    ↓
弹出文件夹选择框，选择已有的 SVN 工作副本目录
    ↓
验证 .svn/entries 文件存在（有效 SVN 工作副本）
    ├── 无效 → 提示「请选择有效的 SVN 工作副本」
    └── 有效 → 自动获取仓库名称（文件夹名）和 URL（svn info）
    ↓
保存仓库到配置列表
    ↓
启动 QFileSystemWatcher + 下行轮询
加载文件列表视图
```

---

## 项目结构

```
SVNFileBox-Qt/
├── CMakeLists.txt
├── resources.qrc
├── SPEC.md
│
├── src/
│   ├── main.cpp                     # 程序入口，QML 引擎初始化，注册 C++ 类型到 QML
│   │
│   ├── svn/
│   │   ├── svnclient.h              # SVN CLI 封装接口
│   │   └── svnclient.cpp
│   │
│   ├── sync/
│   │   ├── syncengine.h              # 同步引擎（上行+下行）
│   │   └── syncengine.cpp
│   │
│   ├── config/
│   │   ├── configservice.h          # 配置读写（JSON）
│   │   └── configservice.cpp
│   │
│   ├── models/
│   │   ├── filemodel.h              # QAbstractListModel 文件列表
│   │   ├── filemodel.cpp
│   │   ├── repository.h             # 仓库数据模型
│   │   ├── repository.cpp
│   │   ├── svnstatus.h              # SVN 状态枚举 + 工具函数
│   │   ├── syncrecord.h             # 同步记录模型
│   │   ├── syncrecorddisplay.h      # 同步记录显示模型（含 emoji 转换）
│   │   └── appconfig.h              # 应用配置模型
│   │
│   ├── utils/
│   │   └── dpapiservice.h           # 密码加密（跨平台：DPAPI/Keychain/libsecret）
│   │
│   └── filewatcher/
│       ├── filewatcherservice.h     # QFileSystemWatcher 封装 + 防抖
│       └── filewatcherservice.cpp
│
├── qml/
│   ├── main.qml                      # 根组件（ApplicationWindow）
│   ├── assets/
│   │   └── (图标资源)
│   ├── components/
│   │   ├── SidebarButton.qml         # 侧边栏按钮组件（卡片样式）
│   │   ├── FileListItem.qml          # 文件列表项（类型图标+名称+状态徽章+大小+时间）
│   │   ├── StatusBadge.qml           # 状态徽章组件（圆角18px彩色圆）
│   │   ├── RepoListItem.qml          # 仓库列表项（图标+名称+路径+X按钮）
│   │   └── FileTypeIcon.qml         # 文件类型 emoji 图标转换器
│   └── pages/
│       ├── MainWindow.qml             # 主窗口（侧边栏 + 路径栏 + 文件列表/同步记录）
│       ├── CheckoutPage.qml           # 从网络添加仓库页面（对话框）
│       ├── AddLocalRepoPage.qml      # 添加本地仓库页面（对话框）
│       ├── SyncRecordsPage.qml       # 同步记录页面（表格视图）
│       ├── SettingsPage.qml          # 设置页面
│       └── AboutPage.qml             # 关于页面
│
└── .github/workflows/
    └── cross-compile.yml             # CI（已有，保持不变）
```

---

## 技术栈

| 层级 | 技术 |
|------|------|
| UI 框架 | Qt 6.5.3 + QML（Qt Quick Controls 2） |
| 后端 | 纯 C++（QtCore） |
| SVN 交互 | SVN CLI（svn 命令行） |
| 配置文件 | JSON（QJsonDocument） |
| 密码加密 | DPAPI（Windows）/ Security Framework（macOS）/ libsecret（Linux） |
| 桌面集成 | QSystemTrayIcon（系统托盘） |
| 构建系统 | CMake + aqtinstall（Qt 安装） |
| CI/CD | GitHub Actions（Linux / macOS / Windows 三平台） |

---

## 配置文件路径

- 配置目录: `~/.svnfilebox/`
- 配置文件: `~/.svnfilebox/config.json`
- 工作副本目录: `~/.svnfilebox/workcopies/`（从网络添加时自动创建）
- 同步记录: `~/.svnfilebox/sync_records/`（按仓库分 .json 文件）
- 日志目录: `~/.svnfilebox/logs/`

---

## 验收标准

1. 添加仓库后，指定本地文件夹能自动监控并同步到 SVN
2. 文件新建/修改/删除，能自动 commit 到 SVN，5秒防抖后触发
3. 服务器端有更新时，60秒内自动检测到并执行 svn update 拉取本地
4. svn update 遇冲突时，按 Last-Write-Wins 处理并记录
5. 同步记录正确显示每次操作的轨迹（包括冲突解决）
6. 主窗口显示文件列表，列样式与 SVNFileBox WPF 一致（类型图标/名称/状态徽章/大小/修改时间）
7. 左侧边栏仓库列表，点击切换仓库，启动/停止对应监控
8. 右键菜单完整：打开文件夹/复制路径/粘贴/新建文件夹/重命名/删除/刷新/手工同步
9. 拖拽文件到列表 → 复制到当前目录 → svn add + commit
10. 窗口关闭最小化到系统托盘，双击托盘恢复

---

## 暂不提供（后续迭代）

- 手动 commit / revert / checkout
- 冲突手动解决界面
- 多文件夹同时监控
- 文件夹级别的 exclude 规则（.gitignore 类）
- 分块同步 / 增量同步
- 同步记录导出
- 多语言 UI（先做中文）

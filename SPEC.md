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
- **多仓库后台监控**：保留最近 N 个活跃仓库的后台监控（LRU 池，可配置），后台仓库轮询间隔更长、通知被抑制

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
- **职责**：配置读写（JSON），仓库列表管理，密码加密备份
- **配置路径**：`~/.svnfilebox/config.json`
- **密码存储**：CredentialStore（机器绑定 XOR+Base64 加密，存到 `~/.svnfilebox/credentials.json`）
  - 防止 config.json 被拷贝到其他机器后密码泄露
  - SVN 实际凭据缓存在 `~/.subversion/`（与 `svn` 命令行共享）
- **新增配置项**：
  - `maxBackgroundRepos`（默认 3）：最大后台监控仓库数量
  - `backgroundPollingInterval`（默认 10 分钟）：后台仓库轮询间隔
- **API**：
  - `loadConfig()` → `AppConfig`
  - `saveConfig(AppConfig)`
  - `addRepository(Repository)`
  - `removeRepository(QString name)`

### 2. SVNClient（libsvn 封装）
- **职责**：封装 libsvn_native 所有 API，所有外部接口仅暴露 Qt 类型
- **实现**：Pimpl 模式，libsvn 类型全部藏在 `struct SVNClient::Private {}` 中，外部只看到 `Private *d`
- **线程安全**：每个 SVNClient 实例独立 `svn_pool_t` + `svn_client_ctx_t`，绝不跨线程共享
- **凭据流程**：
  - 首次操作通过 `setUsername`/`setPassword` 传入
  - libsvn 将凭据缓存到 `~/.subversion/`（与 `svn` 命令行共享）
  - 后续操作无需再传凭据
- **SVN 库链接**：`libsvn_client;libsvn_wc;libsvn_subr;libsvn_repos;libsvn_ra;libsvn_delta`
- **API**（全部返回 Qt 类型，信号参数仅 QString/QStringList/QVariantMap）：
  - `add(path)` → bool
  - `remove(path)` → bool
  - `commit(path, message)` → bool
  - `update(path, rev)` → bool
  - `checkout(url, localPath)` → bool
  - `revert(path, recurse)` → bool
  - `resolveConflict(path, accept)` → bool
  - `cleanup(path)` → bool
  - `move(from, to)` → bool
  - `getStatus(path, depth)` → `QVariantMap`（路径→状态）
  - `getRepoUrl(path)` → QString
  - `getInfo(path)` → QString（XML）
  - `getStatusString(path)` → QString
  - `getWorkingCopyRevision(path)` → int
  - `getHeadRevision(url)` → int
  - `getConflictedFiles(path)` → QStringList
  - `getLastChangedTime(path)` → QString
  - `testConnection(url)` → bool
  - `isCredentialValid(url)` → bool
  - `setUsername(u)` / `setPassword(p)` / `setConfigDir(dir)`
- **信号**：`commandFinished(cmd, path, rev)` / `commandError(err)` / `notify(msg)`
- **错误处理**：`svn_error_t*` 在 Private 边界统一转为 `QString`，绝不泄漏到外部

### 5. RepoGlobalManager（多仓库协调器）
- **职责**：统一管理多个 RepoManager 实例，支持多仓库同时监控
- **LRU 后台池**：`m_backgroundRepos` 维护最近后台化的仓库列表（最多 `maxBackgroundRepos` 个）
  - 切换前台仓库时，旧前台仓库自动 `background()` 降级并加入池首
  - 超过上限时，最老的仓库 `shutdown()` 并从管理器移除
- **切换流程**（`switchToAsync`）：
  1. 旧前台仓库 `background()` → 进入 LRU 池
  2. 若池超限，弹出最老仓库并 `shutdown()`
  3. 解绑旧仓库信号，绑定新仓库信号
  4. 新仓库 `focus()` → 前台监控
- **启动恢复**（`restoreAndSwitchToLastActive`）：从 `config.json` 加载所有仓库，根据 `maxBackgroundRepos` 确定初始前台仓库
- **信号转发**：转发当前活跃仓库的 `filesChanged` / `syncNotification` / `conflictDetected`

### 6. SvnCommandExecutor（SVN 操作队列）
- **职责**：串行化 SVN 写操作（add/remove/commit/update 等），支持去重和超时
- **实现**：独立 worker 线程，`QSemaphore` 控制并发写，dedup 防止同一路径重复操作
- **去重规则**：同一路径的同类操作在队列中只保留最新一条
- **超时处理**：写操作超时后继续等待，不强行终止，30秒安全强制退出
- **API**：
  - `enqueue(cmd, path, ...)` → 添加操作到队列
  - `executeReadOnly(cmd, path, ...)` → 执行只读操作（无需队列）
  - `stop()` / `waitForDrained(ms)` → 停止并等待排空
- **信号**：`onCommandCompleted(result)` / `onTimeout(cmd, path)` / `onAuthError(path)`

### 7. SyncEngine（SyncService）
- **职责**：上行同步（FileWatcher + 防抖）+ 下行同步（轮询）
- **上行**：QFileSystemWatcher 监控 → 5秒防抖 → svn add/commit
- **下行**：定时器轮询（默认60秒前台，10分钟后台）→ svn update → 检测冲突
- **全量扫描**：每30分钟扫描 svn status，补充 FileWatcher 遗漏
- **后台监控模式**：`setBackgroundMode(bool)` 切换轮询间隔，前台60秒/后台10分钟
- **API**：
  - `startSync(Repository repo)`
  - `stopSync()`
  - `syncNow()`（手工同步：先上行 pending 文件，再下行 poll）
  - `status()` → QString（当前状态描述）
  - `setBackgroundMode(bool)`（前台/后台轮询间隔切换）
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

### 8. SyncRecordService
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

### 9. CredentialStore（机器绑定密码加密）
- **职责**：将仓库密码加密存储到 `~/.svnfilebox/credentials.json`，防止配置文件被拷贝到其他机器后密码泄露
- **加密方式**：XOR + Base64，密钥为机器 ID（`/etc/machine-id` 的 SHA256 哈希前 128 位）
- **Key 设计**：`repoName`（仓库显示名，重命名时需迁移）
- **文件路径**：`~/.svnfilebox/credentials.json`
- **API**：
  - `store(repoName, username, password)` → 加密写入
  - `retrieve(repoName)` → 返回 `{username, password}` 或空字符串
  - `remove(repoName)` → 删除条目
  - `has(repoName)` → bool
- **与 SVN 凭据缓存的关系**：CredentialStore 是加密备份，SVN 实际认证依赖 `~/.subversion/` 的原生缓存（与 `svn` 命令行共享）

### 10. FileWatcherService（已集成到 SyncEngine）
- **现状**：QFileSystemWatcher 功能直接集成在 SyncEngine 内部，不再独立服务
- **NotifyFilter**：`FileName | DirectoryName | LastWrite | Size`
- **IncludeSubdirectories**：true（递归监控，Qt 6.7 无 recursive signal，手动 walk 树）
- **防抖**：收集 5 秒内的变化事件，合并为一批后触发
- **重试机制**：FileWatcher 报错时（如目录被删），5秒后尝试重连
- **过滤**：排除 `.svn` 目录内的变化
- **职责**：管理单一仓库的 SVNClient + SyncEngine + SvnCommandExecutor 生命周期
- **状态机**（`RepoState` 枚举）：
  - `None`：初始状态，无监控
  - `Focused`：前台监控，正常轮询（1分钟），所有通知显示
  - `Background`：后台监控，长轮询（10分钟），常规同步通知被抑制
  - `Closed`：停止同步，释放资源
- **通知过滤**：后台模式下 `isRoutineNotification()` 抑制以下常规通知：`批量同步完成` / `已同步: xxx` / `已同步删除: xxx` / `同步失败: xxx` / `同步已启动: xxx`；冲突、损坏等重要事件始终显示
- **API**：
  - `focus()` → 切换到前台监控
  - `background()` → 降级为后台监控
  - `dismiss()` → `background()` 的别名（向后兼容）
  - `shutdown()` → 停止同步并释放资源

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
│   │   ├── svnclient.h              # libsvn Pimpl 封装（仅暴露 Qt 类型）
│   │   ├── svnclient.cpp
│   │   ├── svncommand.h             # SVN 命令枚举 + SvnCommandItem
│   │   ├── svncommand.cpp
│   │   ├── svncommandexecutor.h     # SVN 操作队列（去重 + 超时）
│   │   └── svncommandexecutor.cpp
│   │
│   ├── sync/
│   │   ├── syncengine.h             # 同步引擎（上行+下行+后台模式）
│   │   ├── syncengine.cpp
│   │   ├── syncrecordservice.h      # 同步记录持久化
│   │   ├── syncrecordservice.cpp
│   │   └── commitqueue.h            # 提交队列
│   │
│   ├── config/
│   │   ├── configservice.h          # 配置读写（JSON）
│   │   ├── configservice.cpp
│   │   ├── credentialstore.h        # 机器绑定密码加密存储
│   │   └── credentialstore.cpp
│   │
│   ├── services/
│   │   ├── repomanager.h            # 仓库生命周期管理（Focused/Background/Closed）
│   │   ├── repomanager.cpp
│   │   ├── repoglobalmanager.h     # 多仓库协调器（LRU 后台池）
│   │   └── repoglobalmanager.cpp
│   │
│   ├── models/
│   │   ├── filemodel.h              # QAbstractListModel 文件列表
│   │   ├── filemodel.cpp
│   │   └── ...
│   │
│   └── ui/
│       └── ...
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
| SVN 交互 | libsvn 1.14（原生库，Pimpl 封装） |
| 配置文件 | JSON（QJsonDocument） |
| 密码加密 | 机器绑定 XOR+Base64（CredentialStore）+ SVN 原生凭据缓存 |
| 桌面集成 | QSystemTrayIcon（系统托盘） |
| 构建系统 | CMake + aqtinstall（Qt 安装） |
| CI/CD | GitHub Actions（Linux / macOS / Windows 三平台） |

---

## 配置文件路径

- 配置目录: `~/.svnfilebox/`
- 配置文件: `~/.svnfilebox/config.json`
- 工作副本目录: `~/.svnfilebox/workcopies/`（从网络添加时自动创建）
- 同步记录: `~/.svnfilebox/sync_records/`（按仓库分 .json 文件）
- 凭证备份: `~/.svnfilebox/credentials.json`（机器绑定加密）
- SVN 凭据缓存: `~/.subversion/`（SVN 原生，与 `svn` 命令行共享）
- SVN 隔离配置（可选）: `~/.svnfilebox/svn-configs/<hash>/`
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
11. **多仓库后台监控**：保留前 N 个（`maxBackgroundRepos`，默认 3）最近活跃仓库的后台监控，轮询间隔 10 分钟
12. **后台通知抑制**：后台仓库的常规同步通知（已同步/批量完成等）不显示，冲突/损坏等重要事件仍显示
13. **libsvn 凭据共享**：SVN 操作的用户名密码缓存在 `~/.subversion/`，与 `svn` 命令行共享同一份凭据

---

## 暂不提供（后续迭代）

- 手动 commit / revert / checkout
- 冲突手动解决界面
- 多文件夹同时监控
- 文件夹级别的 exclude 规则（.gitignore 类）
- 分块同步 / 增量同步
- 同步记录导出
- 多语言 UI（先做中文）

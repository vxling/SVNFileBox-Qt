# QML 编码规范

> 本规范用于 SVNFileBox-Qt 项目，写代码和 Code Review 时参考。

## 官方参考

| 文档 | URL |
|------|-----|
| QML Coding Conventions | https://doc.qt.io/qt-6/qtqml-codecoinfo.html |
| QML Best Practices | https://doc.qt.io/qt-6/qtquick-bestpractices.html |
| QML/C++ Integration | https://doc.qt.io/qt-6/qtqml-cppintegration-overview.html |

---

## 1. 语法基础

### 1.1 属性赋值禁止重复

QML 允许同一属性声明两次，第二个值静默覆盖第一个，**不报错**。代码冗余且容易出 bug。

```qml
// ❌ 错误：重复 flat 属性
Button {
    flat: true
    flat: true          // ← 第二个被忽略，但代码冗余
    text: "确认"
}

// ✅ 正确：每个属性只有一条
Button {
    flat: true
    text: "确认"
}
```

### 1.2 属性类型必须匹配

```qml
// ❌ 错误：字符串赋值给数值属性（隐式转换，运行时行为不确定）
width: "100"
height: "200"

// ✅ 正确：使用正确类型
width: 100
height: 200

// ✅ 正确：表达式赋值
width: parent.width - 20
```

### 1.3 导入版本明确化

```qml
// ❌ 模糊版本号（Qt 6 尽量用明确版本）
import QtQuick
import QtQuick.Controls

// ✅ 明确版本号
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
```

---

## 2. 组件结构

### 2.1 根元素类型选择

作为页面加载到 StackView/Loader 时，**用 Item 做根类型**：

```qml
// ✅ 正确
Item {
    id: rootItem
    anchors.fill: parent

    StackView { id: contentStack; anchors.fill: parent }
    // ...
}

// ❌ 不要用 Window、ApplicationWindow 做子页面根元素
// ❌ 不要用 Component {} 包裹页面内容
```

### 2.2 Component {} 嵌套语法（最常见 parse error）

```qml
// ❌ 致命错误：Component 内嵌套大括号定义对象，QML parse error
Component { id: mainPage { mainPageContent{} } }

// ✅ 正确：Item 直接作为页面根元素
Item {
    id: mainPageItem
    anchors.fill: parent
    // ...
}
```

**检查方法**：`grep -rn "Component { id:" qml/`

### 2.3 StackView 页面结构

```qml
// ❌ 错误：页面 Item 是 StackView 的 sibling，全部同时渲染
Item {
    StackView { id: contentStack; anchors.fill: parent }
    Item { id: mainPageItem; anchors.fill: parent }        // ❌ sibling
    Item { id: checkoutPageContent; anchors.fill: parent }  // ❌ sibling
}

// ✅ 正确：页面 Item 放在 StackView 内部
Item {
    StackView {
        id: contentStack
        anchors.fill: parent
        initialItem: mainPageItem

        Item { id: mainPageItem; anchors.fill: parent }
        Item { id: checkoutPageContent; anchors.fill: parent }
    }
}
```

### 2.4 布局属性禁止混用

```qml
// ❌ 错误：anchors 和 x/y/width/height 混用
Rectangle {
    x: 10
    anchors.left: parent.left   // ❌ 冲突

    width: 100
    anchors.fill: parent         // ❌ 冲突
}

// ✅ 正确：二选一
Rectangle {
    anchors.fill: parent
}
// 或
Rectangle {
    x: 10; y: 10
    width: 100; height: 100
}
```

### 2.5 父容器必须有明确尺寸

```qml
// ❌ 错误：依赖隐式尺寸，布局可能错乱
Column {
    // 子元素依赖隐式宽高
}

// ✅ 正确：明确尺寸或填充父容器
Column {
    anchors.fill: parent
    // 或
    width: 300; height: 500
}
```

---

## 3. 命名规范

### 3.1 id 命名（camelCase）

```qml
// ✅ 正确
id: mainPageItem
id: settingsDialog
id: fileListView
id: syncEngine

// ❌ 避免
id: MainPage       // PascalCase 不一致
id: 1stItem        // 以数字开头
id: main-page      // 中划线
id: sync_engine    // 下划线
```

### 3.2 信号处理器命名（on 前缀 + PascalCase）

```qml
// ✅ 正确
onClicked: { }
onActivated: { }
onCurrentItemChanged: { }
onSyncStarted: { }

// ❌ 常见错误
onclick: { }       // 全小写
OnClicked: { }      // 全大写
handleClick: { }    // 缺少 on 前缀
```

---

## 4. 信号与定时器

### 4.1 Connections 的 target 必须存在

```qml
// ⚠️ target 为 null/undefined 时输出 runtime warning
Connections {
    target: someObject  // 如果 someObject 为 null
    function onSignal() { }
}

// ✅ 防御：使用三元表达式或确保 target 存在
Connections {
    target: someObject || null
    function onSignal() { }
}
```

### 4.2 Timer 必须设置触发条件

```qml
// ❌ 错误：Timer 默认 running: false，不触发
Timer {
    interval: 1000
    onTriggered: console.log("never fires")
}

// ✅ 正确
Timer {
    interval: 1000
    running: true
    repeat: true
    onTriggered: console.log("every second")
}
```

---

## 5. 性能优化

### 5.1 大列表必须加 cacheBuffer

```qml
// ❌ 错误：列表大时严重卡顿（一次性创建所有 delegate）
ListView {
    model: 10000
    delegate: Item { ... }
}

// ✅ 正确：预渲染附近项，减少内存占用
ListView {
    model: 10000
    cacheBuffer: 200
    delegate: Item { ... }
}
```

### 5.2 Component.onCompleted 避免重操作

```qml
// ❌ 错误：onCompleted 里做耗时操作，阻塞 UI 启动
Component.onCompleted: {
    loadAllData()  // 可能阻塞几秒
}

// ✅ 正确：延迟到下一帧
Component.onCompleted: {
    Qt.callLater(doHeavyWork)
}
```

### 5.3 Loader 必须管理生命周期

```qml
// ✅ 即用即销毁
Loader {
    id: pageLoader
    active: false   // 默认不激活

    sourceComponent: someComponent
}

onEnterPage: {
    pageLoader.active = true
}

onExitPage: {
    pageLoader.active = false
    pageLoader.sourceComponent = undefined
}
```

---

## 6. Qt Quick Controls 注意事项

### 6.1 Button 自定义背景（Qt 6 必须加 Basic 风格）

```qml
// ❌ 错误：Qt 6 默认 Imagine 风格不支持 background 自定义
Button {
    background: Rectangle { color: "red"; radius: 4 }
    contentItem: Text { text: "Click" }
}

// ✅ 正确：启用 BasicStyle
Button {
    Controls.style: Basic
    background: Rectangle { color: "red"; radius: 4 }
    contentItem: Text { text: "Click" }
}
```

### 6.2 FileDialog Qt 5 → Qt 6 属性迁移

```qml
// ❌ Qt 5 风格（Qt 6 中不存在）
FileDialog {
    folder: shortcuts.home
}

// ✅ Qt 6 风格
FileDialog {
    currentFolder: shortcuts.home
    onAccepted: console.log("Selected:", currentFolder)
}
```

---

## 7. QML + C++ 混合编程

### 7.1 Q_PROPERTY 声明完整性

```cpp
// ❌ 错误：缺少 NOTIFY 信号，QML 无法感知属性变化
Q_PROPERTY(QString name READ name WRITE setName)

// ✅ 正确：每个可写属性都应有 NOTIFY 信号
Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

// setter 也要发送信号
void setName(const QString &name) {
    if (m_name != name) {
        m_name = name;
        emit nameChanged();
    }
}
```

### 7.2 Q_INVOKABLE 方法参数类型

```cpp
// ✅ 推荐：使用 QVariant 或明确类型
Q_INVOKABLE void process(const QVariant &data);
Q_INVOKABLE bool doCheckout(const QString &url, const QString &path,
                              const QString &user, const QString &pass);

// ❌ 避免：QML 不友好的类型（如 QMap<QString, QVariant>）
```

### 7.3 C++ 类型注册顺序

```cpp
// ✅ 在 engine.load() 之前注册
qmlRegisterSingletonType<ConfigService>("SVNFileBox", 1, 0, "ConfigService", &ConfigService::create);
qmlRegisterType<SyncEngine>("SVNFileBox", 1, 0, "SyncEngine");

QQmlApplicationEngine engine;
engine.load(url);

// ❌ 不要在 engine.load() 之后注册
```

### 7.4 枚举类型注册

```cpp
// ✅ Q_ENUM 注册后 QML 可直接使用字符串比较
class SVNClient : public QObject {
    Q_OBJECT
public:
    enum class ErrorLevel { Success, Warning, Error };
    Q_ENUM(ErrorLevel)
};

// QML 中
if (level === SVNClient.ErrorLevel.Warning) { }

// ✅ QML 使用 ErrorLevel 枚举
Connections {
    target: svnClient
    function onCommandWarning(warning) {
        console.warn("SVN warning:", warning)
    }
}
```

---

## 8. 项目特有约定（SVNFileBox-Qt）

### 8.1 Context Property 命名

所有注册为 QML context property 的对象，用小写 camelCase：

```cpp
// main.cpp
engine.rootContext()->setContextProperty("svnClient", svnClient);
engine.rootContext()->setContextProperty("syncEngine", syncEngine);
engine.rootContext()->setContextProperty("configService", configService);
engine.rootContext()->setContextProperty("fileModel", fileModel);
engine.rootContext()->setContextProperty("syncRecordService", syncRecordService);
```

### 8.2 路径变量命名

```qml
// ✅ 使用 repoRoot / currentPath / localPath 明确语义
property string currentPath: fileModel.currentPath
property string repoRoot: configService.localPath() + "/" + repoName

// ❌ 避免
property string path: "..."     // 语义模糊
property string p: "..."        // 缩写
```

### 8.3 SVN 命令行参数规范

```cpp
// ✅ 所有 svn 命令必须包含这些参数
QStringList args = {"command", "--non-interactive", "--trust-server-cert"};
args += additional_args;

// ❌ 不要遗漏 --non-interactive（交互式会挂起）
```

### 8.4 FileSystemWatcher 监控策略

```cpp
// ✅ 只监控当前目录 + 一级子目录（不递归）
void SyncEngine::watchPath(const QString &path) {
    // 1. 销毁旧的 watcher
    if (m_watcher) {
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }
    // 2. 重建 watcher
    m_watcher = new QFileSystemWatcher(this);
    // 3. 监控当前目录
    if (QDir(path).exists()) m_watcher->addPath(path);
    // 4. 监控一级子目录
    QDir dir(path);
    for (const QString &sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString subPath = path + "/" + sub;
        if (QFileInfo(subPath).isDir()) m_watcher->addPath(subPath);
    }
}
```

---

## 9. 快速参考

### 写代码时

```
✅ 属性不重复
✅ 属性类型正确（数值 vs 字符串）
✅ 明确版本号 import
✅ id 用 camelCase
✅ 信号处理器 on 前缀 + PascalCase
✅ anchors 不与 x/y/width/height 混用
✅ Timer 设置 running
✅ 大列表加 cacheBuffer
✅ Button 自定义背景加 Controls.style: Basic
✅ FileDialog 用 currentFolder（非 folder）
✅ Q_PROPERTY 有 NOTIFY
✅ svn 命令加 --non-interactive --trust-server-cert
✅ FileSystemWatcher 不递归，只监控当前目录 + 一级子目录
```

### Code Review 时

```
🔴 致命：Component { id: x { } }  语法错误
🔴 致命：StackView sibling 页面结构
🔴 致命：属性类型不匹配（字符串赋值给数值属性）
⚠️ 警告：target 可能为 null 的 Connections
⚠️ 警告：FileDialog 用 Qt 5 的 folder 属性
⚠️ 警告：Timer 无 running 条件
⚡ 优化：cacheBuffer 缺失
⚡ 优化：Component.onCompleted 含重操作
📝 规范：id 命名不一致
📝 规范：import 版本模糊
```

---

## 10. 本项目历史 Bug 记录

> 下次 Code Review 时优先检查这些 pattern，发现即修复。

### 🔴 P0 — 必须修复

| # | 问题描述 | 根因 | 修复方案 |
|---|---------|------|---------|
| 1 | FileSystemWatcher 只监控 repo 根目录，进入子目录后不监控该子目录 | `watchPath()` 只 add 一次，后续 `navigateInto()` 没有重建 watcher | 每次 `navigateInto()` 调用 `syncEngine.watchPath(path)` 重建监控 |
| 2 | checkout 不传用户名密码 | `SVNClient::checkout()` 签名只有 url + localPath，QML 传了 user/pass 但被丢弃 | 新增 4 参数重载：`checkout(url, path, user, pass)` |
| 3 | currentPath 变化时 QML 无法感知 | `Q_PROPERTY` 缺 `NOTIFY` 信号，`setCurrentPath()` 没 emit | 加 `NOTIFY currentPathChanged`，cpp 里实现并 emit |
| 4 | 新版 SVN 1.14+ 误判非工作副本 | `isValidWorkingCopy()` 只检测 `.svn/entries`，新版用 `.svn/wc.db` | 同时检测 `.svn/entries` 和 `.svn/wc.db` |

### 🟡 P1 — 应该修复

| # | 问题描述 | 根因 | 修复方案 |
|---|---------|------|---------|
| 5 | handleConflicts() 声明了但从未实现 | 方法声明存在但 cpp 无定义，也无调用 | 实现冲突文件清理（.mine/.rOLD/.rNEW/.orig），commit/update 后调用 |
| 6 | SyncRecord 属性变化时 QML 绑定不更新 | 6 个 Q_PROPERTY 全部缺 `NOTIFY` 信号 | 全部加 `NOTIFY refreshed`，构造函数 emit |
| 7 | 冲突文件产生后残留 | commit/update 后没有冲突清理逻辑 | `handleConflicts()` 检测并删除 .r* / .orig / .rej 文件 |

### 🟢 P2 — 建议修复

| # | 问题描述 | 根因 | 修复方案 |
|---|---------|------|---------|
| 8 | SyncRecord 时间戳反序列化后丢失 | `load()` 用 `currentDateTime()` 覆盖存储的时间，`save()` 也没写 ts 字段 | 加 `setTimestamp()`，`load()`/`save()` 处理 ts 字段 |
| 9 | fullScan() 空函数，无实际扫描 | 函数只调了 `retryPending()`，没有 `svn status` 检测 | 调用 `getStatus()` + `retryPending()` + `pollServer()` |
| 10 | SVN 所有错误统一 exitCode != 0，无法区分等级 | `runSvnBool()` 只看 exitCode，无错误分类 | 新增 `runSvnLevel()` 区分 Warning/Error，新增 `commandWarning` 信号 |

### 📝 本项目规范补充

```cpp
// QML 调用 svn 命令时，doCheckout/doAdd 等函数里多传的参数会被忽略（QML 不报错）
// → C++ 签名对不上时先检查 QML 调用侧参数数量
svnClient.checkout(url, configService.localPath() + "/" + name, user, pass)
//                            ↑ QML 传了 user, pass，但旧签名只有 2 参数 → 被静默忽略
```

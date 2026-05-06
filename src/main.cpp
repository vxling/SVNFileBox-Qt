#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "svn/svnclient.h"
#include "sync/syncengine.h"
#include "config/configservice.h"
#include "models/filemodel.h"

// 临时前向声明，等价于 QML_IMPORT_MINIMAL
// 组件在 qrc 里，直接用 qrc URL 加载，不需要 qmldir
static QUrl qrcUrl(const QString &path) { return QUrl(QStringLiteral("qrc:/") + path); }

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("SVNFileBox");
    app.setOrganizationName("vxling");
    app.setApplicationVersion("1.0.0");

    // 注册 C++ 类型到 QML
    qmlRegisterType<SVNClient>("SVNFileBox.SVN", 1, 0, "SVNClient");
    qmlRegisterType<SyncEngine>("SVNFileBox.Sync", 1, 0, "SyncEngine");
    qmlRegisterType<ConfigService>("SVNFileBox.Config", 1, 0, "ConfigService");
    qmlRegisterType<FileModel>("SVNFileBox.Models", 1, 0, "FileModel");

    QQmlApplicationEngine engine;

    // qrc:/ 成为可导入路径，这样 import "qml/components" 就能找到组件
    engine.addImportPath(":/");

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);
    return app.exec();
}

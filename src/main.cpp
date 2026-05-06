#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "svn/svnclient.h"
#include "sync/syncengine.h"
#include "config/configservice.h"
#include "models/filemodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("SVNFileBox");
    app.setOrganizationName("vxling");

    // 注册 C++ 类型到 QML
    qmlRegisterType<SVNClient>("SVNFileBox.SVN", 1, 0, "SVNClient");
    qmlRegisterType<SyncEngine>("SVNFileBox.Sync", 1, 0, "SyncEngine");
    qmlRegisterType<ConfigService>("SVNFileBox.Config", 1, 0, "ConfigService");
    qmlRegisterType<FileModel>("SVNFileBox.Models", 1, 0, "FileModel");

    QQmlApplicationEngine engine;

    // 全局单例
    engine.rootContext()->setContextProperty("svnClient", new SVNClient(&engine));
    engine.rootContext()->setContextProperty("syncEngine", new SyncEngine(&engine));
    engine.rootContext()->setContextProperty("configService", new ConfigService(&engine));

    const QUrl url(QStringLiteral("qrc:/qml/pages/MainWindow.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);
    return app.exec();
}

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "svn/svnclient.h"
#include "sync/syncengine.h"
#include "sync/syncrecordservice.h"
#include "config/configservice.h"
#include "models/filemodel.h"

static QUrl qrcUrl(const QString &path) { return QUrl(QStringLiteral("qrc:/") + path); }

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("SVNFileBox");
    app.setOrganizationName("vxling");
    app.setApplicationVersion("1.0.0");

    // Register C++ types to QML
    qmlRegisterType<SVNClient>("SVNFileBox.SVN", 1, 0, "SVNClient");
    qmlRegisterType<SyncEngine>("SVNFileBox.Sync", 1, 0, "SyncEngine");
    qmlRegisterType<SyncRecord>("SVNFileBox.Sync", 1, 0, "SyncRecord");
    qmlRegisterType<ConfigService>("SVNFileBox.Config", 1, 0, "ConfigService");
    qmlRegisterType<FileModel>("SVNFileBox.Models", 1, 0, "FileModel");

    // Singleton instances
    SVNClient svnClient;
    ConfigService configService;
    FileModel fileModel;
    fileModel.setSvnClient(&svnClient);

    // SyncRecordService: use its static instance
    SyncRecordService *recordService = SyncRecordService::instance();

    // SyncEngine must be constructed after svnClient and recordService are ready
    SyncEngine syncEngine;
    syncEngine.setSvnClient(&svnClient);
    syncEngine.setSyncRecordService(recordService);

    QQmlApplicationEngine engine;

    // Context properties available in all QML files
    engine.rootContext()->setContextProperty("svnClient", &svnClient);
    engine.rootContext()->setContextProperty("configService", &configService);
    engine.rootContext()->setContextProperty("fileModel", &fileModel);
    engine.rootContext()->setContextProperty("syncEngine", &syncEngine);
    engine.rootContext()->setContextProperty("syncRecordService", recordService);

    // qrc:/ becomes an import path
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

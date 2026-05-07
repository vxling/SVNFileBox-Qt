#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <iostream>

#include "src/svn/svnclient.h"
#include "src/sync/syncengine.h"
#include "src/sync/syncrecordservice.h"
#include "src/config/configservice.h"
#include "src/models/filemodel.h"

static QUrl qrcUrl(const QString &path) { return QUrl(QStringLiteral("qrc:/") + path); }

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("SVNFileBox");
    app.setOrganizationName("vxling");
    app.setApplicationVersion("1.0.0");

    std::cout << "=== QML Import Paths ===" << std::endl;
    for (const QString &p : QQmlEngine::importPathList())
        std::cout << "  " << p.toStdString() << std::endl;

    // Register C++ types
    qmlRegisterType<SVNClient>("SVNFileBox.SVN", 1, 0, "SVNClient");
    qmlRegisterType<SyncEngine>("SVNFileBox.Sync", 1, 0, "SyncEngine");
    qmlRegisterType<SyncRecord>("SVNFileBox.Sync", 1, 0, "SyncRecord");
    qmlRegisterType<ConfigService>("SVNFileBox.Config", 1, 0, "ConfigService");
    qmlRegisterType<FileModel>("SVNFileBox.Models", 1, 0, "FileModel");

    SVNClient svnClient;
    ConfigService configService;
    FileModel fileModel;
    fileModel.setSvnClient(&svnClient);
    SyncRecordService *recordService = SyncRecordService::instance();
    SyncEngine syncEngine;
    syncEngine.setSvnClient(&svnClient);
    syncEngine.setSyncRecordService(recordService);

    QQmlApplicationEngine engine;
    engine.addImportPath(":/");
    engine.rootContext()->setContextProperty("svnClient", &svnClient);
    engine.rootContext()->setContextProperty("configService", &configService);
    engine.rootContext()->setContextProperty("fileModel", &fileModel);
    engine.rootContext()->setContextProperty("syncEngine", &syncEngine);
    engine.rootContext()->setContextProperty("syncRecordService", recordService);

    QObject::connect(&engine, &QQmlApplicationEngine::warnings,
        [](const QList<QQmlError> &warnings) {
            std::cout << "\n=== QML WARNINGS ===" << std::endl;
            for (const QQmlError &w : warnings)
                std::cout << "  Line " << w.line() << ": " << w.message().toStdString() << std::endl;
        });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        [](QObject *obj, const QUrl &url) {
            if (!obj)
                std::cout << "FAILED to create: " << url.toString().toStdString() << std::endl;
            else
                std::cout << "Created: " << url.toString().toStdString() << std::endl;
        });

    std::cout << "\n=== Loading qrc:/qml/main.qml ===" << std::endl;
    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        std::cout << "ERROR: rootObjects empty!" << std::endl;
        return 1;
    }

    std::cout << "\n=== Event loop ===" << std::endl;
    return app.exec();
}

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "svn/svnclient.h"
#include "svn/svncommand.h"
#include "svn/svncommandexecutor.h"
#include "sync/syncengine.h"
#include "sync/commitqueue.h"
#include "sync/syncrecordservice.h"
#include "config/configservice.h"
#include "services/repomanager.h"
#include "services/repoglobalmanager.h"
#include "models/filemodel.h"
#include "systemtray/traymanager.h"
#include "services/newfileservice.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("SVNFileBox");
    app.setOrganizationName("vxling");
    app.setApplicationVersion("1.0.0");

    // ── Register C++ types to QML ───────────────────────────────
    qmlRegisterType<SVNClient>("SVNFileBox.SVN", 1, 0, "SVNClient");
    qmlRegisterType<SyncEngine>("SVNFileBox.Sync", 1, 0, "SyncEngine");
    qmlRegisterType<SyncRecord>("SVNFileBox.Sync", 1, 0, "SyncRecord");
    qmlRegisterType<ConfigService>("SVNFileBox.Config", 1, 0, "ConfigService");
    qmlRegisterType<FileModel>("SVNFileBox.Models", 1, 0, "FileModel");
    qmlRegisterType<TrayManager>("SVNFileBox.System", 1, 0, "TrayManager");
    qmlRegisterType<SVNFileBox::RepoManager>("SVNFileBox.Services", 1, 0, "RepoManager");
    qmlRegisterUncreatableType<SVNFileBox::Repository>(
        "SVNFileBox.Services", 1, 0, "Repository",
        QStringLiteral("Repository is created from C++, not QML"));

    // NewFileService singleton
    qmlRegisterSingletonType<NewFileService>("SVNFileBox.Services", 1, 0, "NewFileService",
        [](QQmlEngine *, QJSEngine *) -> QObject * {
            static NewFileService inst;
            return &inst;
        });

    // CommitQueue singleton
    qmlRegisterSingletonType<CommitQueue>("SVNFileBox.Sync", 1, 0, "CommitQueue",
        [](QQmlEngine *, QJSEngine *) -> QObject * {
            return &CommitQueue::instance();
        });

    // SyncRecordService singleton
    qmlRegisterSingletonType<SyncRecordService>("SVNFileBox.Sync", 1, 0, "SyncRecordService",
        [](QQmlEngine *, QJSEngine *) -> QObject * {
            return SyncRecordService::instance();
        });

    // ── Singleton instances ─────────────────────────────────────
    SVNClient svnClient;
    ConfigService configService;
    CommitQueue &commitQueue = CommitQueue::instance();
    SyncRecordService *recordService = SyncRecordService::instance();
    TrayManager trayManager;

    // RepoGlobalManager: central multi-repo coordinator
    SVNFileBox::RepoGlobalManager globalManager;

    // FileModel (uses configService for current repo path)
    FileModel fileModel;
    fileModel.setSvnClient(&svnClient);

    // SyncEngine (created per RepoManager, not as global singleton)
    // Each RepoManager creates its own SyncEngine + SvnCommandExecutor
    // The global SyncEngine below is kept for backward compat with existing QML
    SyncEngine syncEngine;
    syncEngine.setSvnClient(&svnClient);
    syncEngine.setSyncRecordService(recordService);

    // ── QML context properties ──────────────────────────────────
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("svnClient", &svnClient);
    engine.rootContext()->setContextProperty("configService", &configService);
    engine.rootContext()->setContextProperty("fileModel", &fileModel);
    engine.rootContext()->setContextProperty("syncEngine", &syncEngine);
    engine.rootContext()->setContextProperty("syncRecordService", recordService);
    engine.rootContext()->setContextProperty("commitQueue", &commitQueue);
    engine.rootContext()->setContextProperty("trayManager", &trayManager);
    engine.rootContext()->setContextProperty("globalManager", &globalManager);

    // Expose SVNCommand enums to QML
    engine.rootContext()->setContextProperty("SvnCommand_ReadOnly", 0);
    engine.rootContext()->setContextProperty("SvnCommand_LocalWrite", 1);
    engine.rootContext()->setContextProperty("SvnCommand_HeavyWrite", 2);

    // qrc:/ becomes an import path
    engine.addImportPath(":/");

    // On app startup: restore repos from config and switch to last active
    QVariantList savedRepos = configService.repositories();
    if (!savedRepos.isEmpty()) {
        globalManager.restoreAndSwitchToLastActive(savedRepos,
                                                   configService.activeRepositoryName());
    }

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    // On app quit: shutdown all repos
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     &globalManager, &SVNFileBox::RepoGlobalManager::shutdownAll);

    engine.load(url);
    return app.exec();
}
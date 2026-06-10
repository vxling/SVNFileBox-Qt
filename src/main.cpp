#include <QApplication>
#include <QSurfaceFormat>

#include "svn/svnclient.h"
#include "svn/svncommand.h"
#include "svn/svncommandexecutor.h"
#include "sync/syncengine.h"
#include "sync/syncrecordservice.h"
#include "config/configservice.h"
#include "services/repomanager.h"
#include "services/repoglobalmanager.h"
#include "models/filemodel.h"
#include "systemtray/traymanager.h"
#include "services/newfileservice.h"
#include "i18n/translator.h"
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SVNFileBox");
    app.setOrganizationName("vxling");
    app.setApplicationVersion("1.0.0");

    // ── Singleton instances ─────────────────────────────────────
    SVNClient svnClient;
    ConfigService configService;
    SyncRecordService *recordService = SyncRecordService::instance();
    TrayManager trayManager;

    // RepoGlobalManager: central multi-repo coordinator
    SVNFileBox::RepoGlobalManager globalManager;
    globalManager.setConfigService(&configService);

    // FileModel (uses configService for current repo path)
    FileModel fileModel;
    fileModel.setSvnClient(&svnClient);
    fileModel.setGlobalManager(&globalManager);

    // SyncEngine: global instance for backward compat
    SyncEngine syncEngine;
    syncEngine.setSvnClient(&svnClient);
    syncEngine.setSyncRecordService(recordService);

    // P3 #4: install translator BEFORE loading UI. Picks the right .qm
    // based on configService.language() (auto / zh-CN / en).
    SVNFileBox::Translator translator;
    translator.installForLanguage(configService.language());

    // On app startup: restore repos from config and switch to last active
    QVariantList savedRepos = configService.repositories();
    if (!savedRepos.isEmpty()) {
        globalManager.restoreAndSwitchToLastActive(savedRepos,
                                                   configService.activeRepositoryName());
    }

    // Main window (Qt Widgets)
    MainWindow mainWin(&configService, &fileModel, &syncEngine, &globalManager);
    mainWin.show();

    // On app quit: shutdown all repos
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     &globalManager, &SVNFileBox::RepoGlobalManager::shutdownAll);

    // Tray "退出" menu → quit
    QObject::connect(&trayManager, &TrayManager::exitRequested, &app, &QApplication::quit);

    return app.exec();
}
QT += core gui widgets sql network

CONFIG += c++17

TARGET = SVNFileBox
TEMPLATE = app

# ── Compiler settings ────────────────────────────────────────────
win32 {
    # Windows: use UTF-8 with BOM so Chinese strings compile
    msvc {
        QMAKE_CFLAGS += /utf-8
        QMAKE_CXXFLAGS += /utf-8
    }
    DEFINES += _CRT_SECURE_NO_WARNINGS
}

# ── Source files ─────────────────────────────────────────────────
SOURCES += \
    ./src/main.cpp \
    ./src/config/configservice.cpp \
    ./src/models/filemodel.cpp \
    ./src/services/fileanalyzer.cpp \
    ./src/services/filecopier.cpp \
    ./src/services/newfileservice.cpp \
    ./src/services/repoglobalmanager.cpp \
    ./src/services/repomanager.cpp \
    ./src/svn/svnclient.cpp \
    ./src/svn/svncommand.cpp \
    ./src/svn/svncommandexecutor.cpp \
    ./src/sync/commitqueue.cpp \
    ./src/sync/ignorepattern.cpp \
    ./src/sync/sqlitesyncrecordstore.cpp \
    ./src/sync/syncengine.cpp \
    ./src/sync/syncrecordservice.cpp \
    ./src/sync/syncrecord.cpp \
    ./src/systemtray/traymanager.cpp \
    ./src/i18n/translator.cpp \
    ./src/ui/addlocaldialog.cpp \
    ./src/ui/checkoutdialog.cpp \
    ./src/ui/copyprogressdialog.cpp \
    ./src/ui/filecarddelegate.cpp \
    ./src/ui/mainwindow.cpp \
    ./src/ui/newfiledialog.cpp \
    ./src/ui/repolistmodel.cpp \
    ./src/ui/settingsdialog.cpp

# ── Header files ─────────────────────────────────────────────────
HEADERS += \
    ./src/config/configservice.h \
    ./src/models/filemodel.h \
    ./src/services/fileanalyzer.h \
    ./src/services/filecopier.h \
    ./src/services/newfileservice.h \
    ./src/services/repoglobalmanager.h \
    ./src/services/repomanager.h \
    ./src/svn/svnclient.h \
    ./src/svn/svncommand.h \
    ./src/svn/svncommandexecutor.h \
    ./src/systemtray/traymanager.h \
    ./src/i18n/translator.h \
    ./src/sync/commitqueue.h \
    ./src/sync/ignorepattern.h \
    ./src/sync/sqlitesyncrecordstore.h \
    ./src/sync/syncengine.h \
    ./src/sync/syncrecordservice.h \
    ./src/sync/syncrecord.h \
    ./src/ui/addlocaldialog.h \
    ./src/ui/checkoutdialog.h \
    ./src/ui/copyprogressdialog.h \
    ./src/ui/filecarddelegate.h \
    ./src/ui/mainwindow.h \
    ./src/ui/newfiledialog.h \
    ./src/ui/repolistmodel.h \
    ./src/ui/settingsdialog.h

# ── Translation files ────────────────────────────────────────────
TRANSLATIONS += \
    ./src/i18n/zh_CN.ts

# ── UI Designer forms ──────────────────────────────────────────────
FORMS += \
    ./src/ui/settingsdialog.ui \
    ./src/ui/addlocaldialog.ui \
    ./src/ui/checkoutdialog.ui \
    ./src/ui/newfiledialog.ui \
    ./src/ui/newfolderdialog.ui

# ── Resources ─────────────────────────────────────────────────────
RESOURCES += \
    ./resources.qrc

# ── Include paths ─────────────────────────────────────────────────
INCLUDEPATH += \
    ./src \
    ./src/config \
    ./src/models \
    ./src/services \
    ./src/svn \
    ./src/sync \
    ./src/systemtray \
    ./src/i18n \
    ./src/ui

# ── Windows-specific ─────────────────────────────────────────────
win32 {
    LIBS += -lshell32 -lcomdlg32 -ladvapi32
}
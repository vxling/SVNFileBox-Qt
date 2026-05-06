QT += core gui qml quick quickcontrols2

CONFIG += c++17

# QML imports
QT_QMLIMPORTS += SVNFileBox.SVN SVNFileBox.Sync SVNFileBox.Config SVNFileBox.Models

SOURCES += \
    src/main.cpp \
    src/svn/svnclient.cpp \
    src/sync/syncengine.cpp \
    src/config/configservice.cpp \
    src/models/filemodel.cpp

HEADERS += \
    src/svn/svnclient.h \
    src/sync/syncengine.h \
    src/config/configservice.h \
    src/models/filemodel.h

RESOURCES += \
    resources.qrc

# Install
target.path = $$PWD/release/bin
INSTALLS += target

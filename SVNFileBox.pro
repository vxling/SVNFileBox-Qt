QT += core gui qml quick quickcontrols2

CONFIG += c++17

SOURCES += \
    src/main.cpp \
    src/svn/svnclient.cpp \
    src/sync/syncengine.cpp \
    src/sync/syncrecordservice.cpp \
    src/sync/syncrecord.cpp \
    src/config/configservice.cpp \
    src/models/filemodel.cpp

HEADERS += \
    src/svn/svnclient.h \
    src/sync/syncengine.h \
    src/sync/syncrecordservice.h \
    src/sync/syncrecord.h \
    src/config/configservice.h \
    src/models/filemodel.h

RESOURCES += \
    resources.qrc

cmake_minimum_required(VERSION 3.16)
project(SVNFileBox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt Core & QML
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Xml Sql Concurrent LinguistTools)

# App
set(APP_SOURCES
    src/main.cpp
    src/svn/svnclient.cpp
    src/svn/svncommand.cpp
    src/svn/svncommandexecutor.cpp
    src/sync/syncengine.cpp
    src/sync/syncrecordservice.cpp
    src/sync/syncrecord.h
    src/sync/sqlitesyncrecordstore.h
    src/sync/sqlitesyncrecordstore.cpp
    src/sync/commitqueue.cpp
    src/sync/ignorepattern.cpp
    src/sync/ignorepattern.h
    src/config/configservice.cpp
    src/services/repomanager.cpp
    src/services/repoglobalmanager.cpp
    src/services/newfileservice.cpp
    src/services/fileanalyzer.cpp
    src/services/filecopier.cpp
    src/i18n/translator.cpp
    src/i18n/translator.h
    src/models/filemodel.cpp
    src/systemtray/traymanager.cpp
)

# MOC must see Q_OBJECT headers
set_source_files_properties(src/sync/syncrecord.h PROPERTIES SKIP_MOC OFF)

add_executable(${PROJECT_NAME} ${APP_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE YES
    MACOSX_BUNDLE_PRODUCT_BUNDLE_IDENTIFIER com.svnfilebox.app
    MACOSX_BUNDLE_BUNDLE_NAME SVNFileBox
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Qt linking
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Xml
    Qt6::Core
    Qt6::Concurrent
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets
    Qt6::QuickControls2
    Qt6::Sql
)

# Resources
target_sources(${PROJECT_NAME} PRIVATE resources.qrc)

# P3 #4: i18n. Wrap hard-coded UI strings in qsTr() in QML; lupdate extracts
# them into i18n/zh_CN.ts / i18n/en.ts. We ship pre-compiled .qm files
# inside the binary; the Translator C++ class picks the right one at
# startup based on ConfigService::language() ("zh-CN" / "en" / "auto").
# RESOURCE_PREFIX /i18n means the .qm files land at qrc:/i18n/zh_CN.qm etc.
qt_add_translations(${PROJECT_NAME}
    TS_FILES i18n/zh_CN.ts i18n/en.ts
    RESOURCE_PREFIX /i18n
)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# CPack config (must set vars BEFORE include)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)

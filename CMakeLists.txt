cmake_minimum_required(VERSION 3.16)
project(SVNFileBox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt Core & QML
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2)
find_package(ZLIB)

# App
set(APP_SOURCES
    src/main.cpp
    src/svn/svnclient.cpp
    src/svn/svncommand.cpp
    src/svn/svncommandexecutor.cpp
    src/sync/syncengine.cpp
    src/sync/syncrecordservice.cpp
    src/sync/syncrecord.h
    src/sync/commitqueue.cpp
    src/config/configservice.cpp
    src/services/repomanager.cpp
    src/services/repoglobalmanager.cpp
    src/services/newfileservice.cpp
    src/models/filemodel.cpp
    src/systemtray/traymanager.cpp
)

# MOC must see Q_OBJECT headers
set_source_files_properties(src/sync/syncrecord.h PROPERTIES SKIP_MOC OFF)

add_executable(${PROJECT_NAME} ${APP_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Qt linking
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets
    Qt6::QuickControls2
    ZLIB::ZLIB
)

# Resources
target_sources(${PROJECT_NAME} PRIVATE resources.qrc)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# CPack config (must set vars BEFORE include)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)

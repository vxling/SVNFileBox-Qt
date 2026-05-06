cmake_minimum_required(VERSION 3.16)
project(SVNFileBox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt Core & QML
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

# App
set(APP_SOURCES
    src/main.cpp
    src/svn/svnclient.cpp
    src/sync/syncengine.cpp
    src/config/configservice.cpp
    src/models/filemodel.cpp
)

add_executable(${PROJECT_NAME} ${APP_SOURCES})

# Qt linking
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

# Resources
target_sources(${PROJECT_NAME} PRIVATE resources.qrc)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# CPack config
include(CPack)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

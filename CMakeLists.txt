cmake_minimum_required(VERSION 3.16)
project(SVNFileBox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt Core & QML
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Xml Sql Concurrent LinguistTools)

# SVN libsvn (libsvn_client + APR runtime)
# Linux: use pkg-config; Windows: use vcpkg find_path/find_library (no CMake config in manifest mode)
if(WIN32)
    # APR/libsvn macros (required before including any APR headers)
    add_definitions(-DAPR_DECLARE_STATIC)
    add_definitions(-DAPR_WANT_MEMFUNCTIONS)
    # Zlib macro
    add_definitions(-DZLIB_WINAPI)

    set(VCPKG_INSTALLED "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows")
    # Direct hardcoded paths for vcpkg manifest mode
    set(SVN_CLIENT_INCLUDE_DIRS "${VCPKG_INSTALLED}/include")
    find_path(SVN_INCLUDE_DIR NAMES svn_client.h PATHS "${VCPKG_INSTALLED}/include/subversion-1" NO_DEFAULT_PATH)
    find_library(SVN_CLIENT_LIB NAMES libsvn_client-1 PATHS "${VCPKG_INSTALLED}/lib" NO_DEFAULT_PATH)
    find_library(SVN_WC_LIB NAMES libsvn_wc-1 PATHS "${VCPKG_INSTALLED}/lib" NO_DEFAULT_PATH)
    find_library(SVN_SUBR_LIB NAMES libsvn_subr-1 PATHS "${VCPKG_INSTALLED}/lib" NO_DEFAULT_PATH)
    find_library(SVN_REPOS_LIB NAMES libsvn_repos-1 PATHS "${VCPKG_INSTALLED}/lib" NO_DEFAULT_PATH)
    find_library(SVN_RA_LIB NAMES libsvn_ra-1 PATHS "${VCPKG_INSTALLED}/lib" NO_DEFAULT_PATH)
    find_library(SVN_DELTA_LIB NAMES libsvn_delta-1 PATHS "${VCPKG_INSTALLED}/lib" NO_DEFAULT_PATH)
    set(SVN_CLIENT_LIBRARIES
        ${SVN_CLIENT_LIB} ${SVN_WC_LIB} ${SVN_SUBR_LIB} ${SVN_REPOS_LIB} ${SVN_RA_LIB} ${SVN_DELTA_LIB}
    )
    message(STATUS "SVN include: ${SVN_INCLUDE_DIR}")
    message(STATUS "SVN lib: ${SVN_CLIENT_LIB}")
    message(STATUS "SVN: using vcpkg manual find_path (Windows)")
else()
    set(PKG_CONFIG_PATH_SVN "/usr/lib/x86_64-linux-gnu/pkgconfig" CACHE STRING "")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SVN_CLIENT REQUIRED libsvn_client libsvn_wc libsvn_subr libsvn_repos libsvn_ra libsvn_delta)
    message(STATUS "SVN include dirs: ${SVN_CLIENT_INCLUDE_DIRS}")
    message(STATUS "SVN libraries:    ${SVN_CLIENT_LIBRARIES}")
endif()

# Zip compression library (cross-platform)
# Linux: use find_path/find_library; Windows: use vcpkg find_path/find_library (no CMake config)
if(WIN32)
    set(VCPKG_INSTALLED "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows")
    find_path(ZIP_INCLUDE_DIR NAMES zip.h PATHS "${VCPKG_INSTALLED}/include" NO_DEFAULT_PATH)
    find_library(ZIP_LIBRARY NAMES zip PATHS "${VCPKG_INSTALLED}/lib" NO_DEFAULT_PATH)
    include_directories(${ZIP_INCLUDE_DIR})
    message(STATUS "ZIP: using vcpkg manual find_path (Windows)")
else()
    find_path(ZIP_INCLUDE_DIR NAMES zip.h PATHS /usr/include NO_DEFAULT_PATH)
    find_library(ZIP_LIBRARY NAMES zip PATHS /usr/lib/x86_64-linux-gnu NO_DEFAULT_PATH)
    include_directories(${ZIP_INCLUDE_DIR})
    link_directories(/usr/lib/x86_64-linux-gnu)
    message(STATUS "ZIP include: ${ZIP_INCLUDE_DIR}")
    message(STATUS "ZIP library: ${ZIP_LIBRARY}")
endif()

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
    src/config/configservice.h
    src/config/credentialstore.cpp
    src/config/credentialstore.h
    src/services/repomanager.cpp
    src/services/repoglobalmanager.cpp
    src/services/newfileservice.cpp
    src/services/fileanalyzer.cpp
    src/services/filecopier.cpp
    src/i18n/translator.cpp
    src/i18n/translator.h
    src/models/filemodel.cpp
    src/systemtray/traymanager.cpp
    # UI (Qt Widgets)
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/repolistmodel.cpp
    src/ui/repolistmodel.h
    src/ui/repoitemdelegate.cpp
    src/ui/repoitemdelegate.h
    src/ui/settingsdialog.cpp
    src/ui/settingsdialog.h
    src/ui/checkoutdialog.cpp
    src/ui/checkoutdialog.h
    src/ui/addlocaldialog.cpp
    src/ui/addlocaldialog.h
    src/ui/newfiledialog.cpp
    src/ui/newfiledialog.h
    src/ui/copyprogressdialog.cpp
    src/ui/copyprogressdialog.h
    src/ui/filecarddelegate.cpp
    src/ui/filecarddelegate.h
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

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SVN_CLIENT_INCLUDE_DIRS}
    ${SVN_CLIENT_INCLUDE_DIRS}/subversion-1
)

# Qt linking
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Concurrent
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets
    Qt6::QuickControls2
    Qt6::Sql
    ${SVN_CLIENT_LIBRARIES}
    ${ZIP_LIBRARY}
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

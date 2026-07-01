cmake_minimum_required(VERSION 3.16)
project(SVNFileBox VERSION 1.0.0 LANGUAGES CXX)

# MSVC doesn't understand GCC's __attribute__(()), erase it so both
# the C++ compiler and Qt's moc (which runs as an independent subprocess)
# see a no-op.
if(MSVC)
    # Write a header that maps __attribute__ to nothing for Qt's moc
    # (moc runs as an independent subprocess and doesn't see /FI).
    set(_attr_h "${CMAKE_BINARY_DIR}/CMakeFiles/__attr.h")
    file(WRITE "${_attr_h}" "#define __attribute__(x)\n")
    set(CMAKE_AUTOMOC_MOC_OPTIONS "--include;${_attr_h}")
    # C3646: MSVC in C++ mode misinterprets C struct members (e.g. 'offset')
    # as C++/CX override specifiers. /wd3646 suppresses this false positive.
    add_compile_options(/wd3646)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt Core & QML
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Xml Sql Concurrent LinguistTools)

#
# SVN libsvn — use subversion-prebuild SDK (https://github.com/vxling/subversion-prebuild)
# SDK archives: svn-sdk-windows-x64.zip, svn-sdk-linux-x64.tar.gz, svn-sdk-macos-x64.tar.gz
# Extract to deps/svn-sdk/{win32,linux,macos}/
#
set(SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/svn-sdk")

if(WIN32)
    # ============================================================
    # Windows: use prebuilt SDK from deps/svn-sdk/win32
    # ============================================================
    add_definitions(-DAPR_DECLARE_STATIC)
    add_definitions(-DAPR_WANT_MEMFUNCTIONS)
    add_definitions(-DZLIB_WINAPI)

    set(SDK_WIN "${SDK_ROOT}/win32")
    set(SVN_CLIENT_INCLUDE_DIRS "${SDK_WIN}/include")
    link_directories("${SDK_WIN}/lib")
    link_directories("${SDK_WIN}/bin")
    find_path(SVN_INCLUDE_DIR NAMES svn_client.h PATHS "${SDK_WIN}/include/subversion-1" NO_DEFAULT_PATH)

    set(SVN_CLIENT_LIBRARIES
        libsvn_client-1 libsvn_wc-1 libsvn_subr-1 libsvn_repos-1 libsvn_ra-1 libsvn_delta-1
        libapr-1 libaprutil-1 libexpat libserf-1
        libcrypto libssl z sqlite3
    )
    message(STATUS "SVN: using prebuilt SDK (Windows) -> ${SDK_WIN}")
    message(STATUS "SVN include: ${SVN_INCLUDE_DIR}")

    # Zip — from SDK
    # zip.h needs zipconf.h which is not shipped in the SDK; generate a minimal stub.
    set(_zipconf_h "${SDK_WIN}/include/zipconf.h")
    if(NOT EXISTS "${_zipconf_h}")
        file(WRITE "${_zipconf_h}"
            "#ifndef ZIPCONF_H\n"
            "#define ZIPCONF_H\n"
            "/* Minimal zipconf.h stub for MSVC build */\n"
            "#define ZIP_EXTERN __declspec(dllimport)\n"
            "#endif\n")
    endif()
    find_path(ZIP_INCLUDE_DIR NAMES zip.h PATHS "${SDK_WIN}/include" NO_DEFAULT_PATH)
    find_library(ZIP_LIBRARY NAMES zip PATHS "${SDK_WIN}/lib" NO_DEFAULT_PATH)
    include_directories(${ZIP_INCLUDE_DIR})
    message(STATUS "ZIP: using prebuilt SDK (Windows)")

elseif(APPLE)
    # ============================================================
    # macOS: use prebuilt SDK from deps/svn-sdk/macos
    # APR headers are symlinked into the SDK during CI (see workflow)
    # ============================================================
    set(SDK_MACOS "${SDK_ROOT}/macos")
    set(ENV{PKG_CONFIG_PATH} "${SDK_MACOS}/lib/pkgconfig")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SVN_CLIENT REQUIRED
        libsvn_client libsvn_wc libsvn_subr libsvn_repos libsvn_ra libsvn_delta)
    message(STATUS "SVN: using prebuilt SDK (macOS) -> ${SDK_MACOS}")
    message(STATUS "SVN include dirs: ${SVN_CLIENT_INCLUDE_DIRS}")
    message(STATUS "SVN libraries:    ${SVN_CLIENT_LIBRARIES}")

    # brew on ARM macOS uses /opt/homebrew, on Intel uses /usr/local
    set(_brew_prefix "/opt/homebrew")
    if(NOT EXISTS "${_brew_prefix}/lib/pkgconfig")
        set(_brew_prefix "/usr/local")
    endif()

    # SDK's libsvn has /usr/include/apr-1 hardcoded; add brew APR so compiler finds it
    include_directories("${_brew_prefix}/include/apr-1" "${_brew_prefix}/include/aprutil-1")

    # Zip — from SDK (or system fallback)
    set(ZIP_ENV_PATH "${SDK_MACOS}/lib/pkgconfig:${_brew_prefix}/lib/pkgconfig")
    set(ENV{PKG_CONFIG_PATH} "${ZIP_ENV_PATH}")
    pkg_check_modules(ZIP_CLIENT QUIET IMPORTED_TARGET libzip)
    if(ZIP_CLIENT_FOUND)
        set(ZIP_LIBRARY PkgConfig::ZIP_CLIENT)
        message(STATUS "ZIP: using prebuilt SDK (macOS) via pkg-config")
    else()
        find_path(ZIP_INCLUDE_DIR NAMES zip.h PATHS "${SDK_MACOS}/include" "${_brew_prefix}/include" NO_DEFAULT_PATH)
        find_library(ZIP_LIBRARY NAMES zip PATHS "${SDK_MACOS}/lib" "${_brew_prefix}/lib" NO_DEFAULT_PATH)
        message(STATUS "ZIP: fallback find_path (macOS) include=${ZIP_INCLUDE_DIR} lib=${ZIP_LIBRARY}")
    endif()

else()
    # ============================================================
    # Linux: use prebuilt SDK from deps/svn-sdk/linux
    # ============================================================
    # pkg-config returns libsvn_ra_serf as a REQUIRED dependency, but serf-1 dev
    # package no longer exists on Ubuntu 24.04 (only the runtime libserf-1.so.1
    # is available). We build the library list manually, excluding serf.
    set(SDK_LINUX "${SDK_ROOT}/linux")
    set(ENV{PKG_CONFIG_PATH} "${SDK_LINUX}/lib/pkgconfig")

    # Find APR libraries from system (dev packages still exist)
    find_library(APR_LIBRARY NAMES apr-1 REQUIRED)
    find_library(APRU_LIBRARY NAMES aprutil-1 REQUIRED)

    # Find SVN SDK libraries manually (avoids serf-1 which is unavailable as dev pkg)
    set(_svn_libs svn_client-1 svn_wc-1 svn_subr-1 svn_repos-1 svn_ra-1 svn_delta-1)
    set(SVN_CLIENT_LIBRARIES "")
    foreach(_lib ${_svn_libs})
        find_library(_lib_path NAMES ${_lib} PATHS "${SDK_LINUX}/lib" NO_DEFAULT_PATH)
        if(_lib_path)
            list(APPEND SVN_CLIENT_LIBRARIES ${_lib_path})
        endif()
    endforeach()
    list(APPEND SVN_CLIENT_LIBRARIES ${APR_LIBRARY} ${APRU_LIBRARY} expat z crypto ssl)

    # SDK's .so files are in SDK_LINUX/lib
    link_directories(${SDK_LINUX}/lib)

    # Discover include path from a known SDK header
    find_path(SVN_INCLUDE_DIR NAMES svn_client.h PATHS "${SDK_LINUX}/include/subversion-1" NO_DEFAULT_PATH)
    if(SVN_INCLUDE_DIR)
        include_directories(${SVN_INCLUDE_DIR})
    endif()
    # Also add system APR include path (SDK's libsvn headers include <apr.h> from system)
    find_path(APR_INCLUDE_DIR NAMES apr.h PATHS /usr/include NO_DEFAULT_PATH)
    if(APR_INCLUDE_DIR)
        include_directories(${APR_INCLUDE_DIR})
        message(STATUS "APR include: ${APR_INCLUDE_DIR}")
    endif()
    message(STATUS "SVN include: ${SVN_INCLUDE_DIR}")

    # Zip — from SDK first, fallback to system
    find_path(ZIP_INCLUDE_DIR NAMES zip.h PATHS "${SDK_LINUX}/include" NO_DEFAULT_PATH)
    if(NOT ZIP_INCLUDE_DIR)
        find_path(ZIP_INCLUDE_DIR NAMES zip.h)
    endif()
    find_library(ZIP_LIBRARY NAMES zip PATHS "${SDK_LINUX}/lib" NO_DEFAULT_PATH)
    if(NOT ZIP_LIBRARY)
        find_library(ZIP_LIBRARY NAMES zip)
    endif()
    if(ZIP_INCLUDE_DIR)
        include_directories(${ZIP_INCLUDE_DIR})
    endif()
    message(STATUS "ZIP: include=${ZIP_INCLUDE_DIR} lib=${ZIP_LIBRARY}")
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

# Deploy Qt runtime dependencies (DLLs, plugins) to the build directory.
# Qt provides qt_deploy_runtime_dependencies() but it only works during install time.
# Use install(SCRIPT) to run a separate cmake script that sets up the deploy function.
install(SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/deploy_runtime.cmake")

# CPack config (must set vars BEFORE include)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)

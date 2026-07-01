# Qt runtime deployment script
# Called via: cmake --install <builddir> --prefix <install_dir>
# Deploys Qt libraries, plugins, and translations next to the executable.
# Supports Linux, Windows, and macOS.
#
# Usage:
#   cmake --install build --prefix build                   # relative path
#   cmake --install build --prefix /tmp/myapp              # absolute path
#   cmake --install build --prefix /full/path/to/myapp     # any location

set(__QT_DEPLOY_IS_SHARED_LIBS_BUILD ON)
set(__QT_NO_CREATE_VERSIONLESS_FUNCTIONS OFF)
set(__QT_DEFAULT_MAJOR_VERSION "6")

# Qt6Core_DIR is not passed through to install script context, so read it from
# the build directory's CMakeCache.txt. This works on all platforms.
set(_qt6_core_dir "")
set(_cmake_cache_file "${CMAKE_BINARY_DIR}/CMakeCache.txt")
if(EXISTS "${_cmake_cache_file}")
    file(STRINGS "${_cmake_cache_file}" _cache_lines)
    foreach(_line IN LISTS _cache_lines)
        if(_line MATCHES "^Qt6Core_DIR:PATH=(.*)")
            set(_qt6_core_dir "${CMAKE_MATCH_1}")
            break()
        endif()
    endforeach()
endif()

# Final fallback to common Linux Qt6 installation path
if(_qt6_core_dir STREQUAL "")
    set(_qt6_core_dir "/home/osuser/Qt/6.7.2/6.7.2/gcc_64/lib/cmake/Qt6Core")
endif()

# Qt deploy support files — when cmake --install is given an absolute path,
# CMAKE_BINARY_DIR resolves to the actual build directory
set(_qt_deploy_support_dir "${CMAKE_BINARY_DIR}/.qt")

if(NOT EXISTS "${_qt_deploy_support_dir}/QtDeploySupport.cmake")
    message(FATAL_ERROR "Qt deploy support not found at: ${_qt_deploy_support_dir}")
endif()
if(NOT EXISTS "${_qt6_core_dir}/Qt6CoreDeploySupport.cmake")
    message(FATAL_ERROR "Qt6CoreDeploySupport.cmake not found at: ${_qt6_core_dir}")
endif()

include(${_qt_deploy_support_dir}/QtDeploySupport.cmake)
include(${_qt6_core_dir}/Qt6CoreDeploySupport.cmake)

# Resolve install directory to absolute path.
# CMAKE_INSTALL_PREFIX is set by --prefix argument to cmake --install.
get_filename_component(_install_dir "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)

# Override Qt's deploy prefix so paths resolve correctly
set(QT_DEPLOY_PREFIX "${_install_dir}")

# Executable name - override with -DAPP_NAME=YourApp at cmake --install time if needed
if(NOT DEFINED _app_name)
    set(_app_name "SVNFileBox")
endif()
get_filename_component(_exe_path "${_install_dir}/bin/${_app_name}" ABSOLUTE)

message(STATUS "Deploying Qt runtime for '${_app_name}' to: ${_install_dir}")

qt_deploy_runtime_dependencies(
    EXECUTABLE "${_exe_path}"
    BIN_DIR    "${_install_dir}"
    LIB_DIR    "${_install_dir}"
    QML_DIR    "${_install_dir}/qml"
    PLUGINS_DIR "${_install_dir}/plugins"
)

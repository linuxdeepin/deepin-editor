cmake_minimum_required(VERSION 3.16)
project(deepin-editor-tests VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(UnitTestUtils)

enable_testing()

# Google Test (system package: libgtest-dev 1.12.1)
find_package(GTest REQUIRED)
include(GoogleTest)

# 代码覆盖率：Debug 模式下启用 gcov 插桩
# test-prj-running.sh 的 lcov 采集步骤依赖此标志采集覆盖率数据
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -O0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov")
    message(STATUS "UT: Code coverage enabled (gcov instrumentation)")
endif()

# Third-party packages (detected from project)
# Qt 6.8.0 + DTK6 + KF6 + ICU + chardet，与 src/CMakeLists.txt 保持一致
find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Widgets DBus Concurrent PrintSupport Svg Xml
    WebEngineWidgets WebChannel OpenGLWidgets Core5Compat)
find_package(Dtk6 REQUIRED COMPONENTS Widget Core)
find_package(KF6Codecs REQUIRED)
find_package(KF6SyntaxHighlighting REQUIRED)
find_package(ICU COMPONENTS i18n uc REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(chardet REQUIRED chardet)
include_directories(${chardet_INCLUDE_DIRS})

# Test subdirectories
add_subdirectory(editor_markdown)
add_subdirectory(common)
add_subdirectory(startmanager)
add_subdirectory(common2)
add_subdirectory(daemon_ut)
add_subdirectory(thememodule)
add_subdirectory(editor_areas)
add_subdirectory(editor_undo)
add_subdirectory(editor_wrapper)
add_subdirectory(editor_core)
add_subdirectory(controls_ut)
add_subdirectory(widgets_ut)
add_subdirectory(app_ut)

message(STATUS "UT: Unit tests configuration complete")

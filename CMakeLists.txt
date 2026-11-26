cmake_minimum_required(VERSION 3.7)

set(VERSION "6.0.0" CACHE STRING "Application version")

project(deepin-editor
    VERSION ${VERSION}
    DESCRIPTION "Text editor for DDE"
    HOMEPAGE_URL "https://github.com/linuxdeepin/deepin-editor"
    LANGUAGES CXX C
)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message("Enable build hardening.")

    set(CMAKE_VERBOSE_MAKEFILE ON)

    set(HARDENING_FLAGS "-Wdate-time -D_FORTIFY_SOURCE=2 -g -O2 -ffile-prefix-map=${CMAKE_SOURCE_DIR}=. -fstack-protector-strong -fstack-clash-protection -Wformat -Werror=format-security")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${HARDENING_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${HARDENING_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now")
endif()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "sw_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee")
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
# 引入翻译生成
include(translation-generate)

# Find Qt version
# 不能仅凭"存在 Qt6"就选择 Qt6：mips64el 源没有 Qt6WebEngine 包，但其它依赖
# （如 polkit-qt6）可能把 qt6-base-dev/qmake6 带进构建环境，形成"部分 Qt6"。
# 只有当应用必需的组件集（以 WebEngineWidgets 为代表）在 Qt6 下齐全时才选
# Qt6，否则回退 Qt5（QOpenGLWidget 头文件两代一致，代码可双栈编译）。
find_package(Qt6 QUIET COMPONENTS Core WebEngineWidgets)
if (Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    message("   >>> Found Qt version: 6 (full stack with WebEngineWidgets)")
else()
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS Core)
    message("   >>> Found Qt version: ${QT_VERSION_MAJOR} (Qt6 stack incomplete, falling back)")
endif()
set(QT_DESIRED_VERSION ${QT_VERSION_MAJOR})

if (QT_VERSION_MAJOR MATCHES 6)
    set(DTK_VERSION_MAJOR 6)
    set(KF_VERSION_MAJOR 6)
else()
    set(DTK_VERSION_MAJOR "")
    set(KF_VERSION_MAJOR 5)
endif()
message("   >>> Build with DTK: ${DTK_VERSION_MAJOR}")

# 设置软件版本
include_directories(${PROJECT_BINARY_DIR})
configure_file(${PROJECT_SOURCE_DIR}/src/environments.h.in ${PROJECT_BINARY_DIR}/environments.h @ONLY)

# 玲珑路径配置
add_definitions(-DLINGLONG_PREFIX=\"${CMAKE_INSTALL_PREFIX}/\")
# 高亮文件路径配置
set(KF5_HIGHLIGHT_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/share/deepin-editor/org.kde.syntax-highlighing)
add_definitions(-DKF5_HIGHLIGHT_PATH=\"${KF5_HIGHLIGHT_INSTALL_PATH}\")

add_subdirectory(src)

option(BUILD_TESTS "Build unit tests" ON)
# Release 模式（打包构建）不编译单元测试，避免 gtest_discover_tests 在构建机上超时
if(BUILD_TESTS AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    add_subdirectory(tests)
endif()

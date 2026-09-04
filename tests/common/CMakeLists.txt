# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# tests/common —— src/common 模块单元测试（B2 批次）
# 每个类一个独立测试目标：test_settings / test_config / test_utils / test_urlinfo / test_eventlog

set(QT_VERSION 6)
set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

# QSignalSpy 需要 QtTest；根 tests/CMakeLists.txt 未包含该组件，此处补找
find_package(Qt6 REQUIRED COMPONENTS Test)
set(COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/widgets
    ${CMAKE_SOURCE_DIR}/src/controls
    ${CMAKE_SOURCE_DIR}/src/basepub
    ${CMAKE_SOURCE_DIR}/tests/3rdparty/stub
)

# ---------------------------------------------------------------------------
# 测试资源：qss / svg / encodes.ini / settings.json（由项目模板生成，路径回填 qrc）
# ---------------------------------------------------------------------------
set(THEME_DEFAULT_PATH "/usr/share/deepin-editor/themes/deepin.theme")
set(TEST_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_data)
configure_file(
    "${CMAKE_SOURCE_DIR}/src/resources/settings.json.in"
    "${TEST_DATA_DIR}/settings.json"
    @ONLY
)
configure_file("test_data/test.qss" "${TEST_DATA_DIR}/test.qss" COPYONLY)
configure_file("test_data/test.svg" "${TEST_DATA_DIR}/test.svg" COPYONLY)
configure_file("test_data/encodes.ini" "${TEST_DATA_DIR}/encodes.ini" COPYONLY)
configure_file("ut_resources.qrc.in" "${CMAKE_CURRENT_BINARY_DIR}/ut_resources.qrc" @ONLY)

set(UT_RESOURCES_QRC ${CMAKE_CURRENT_BINARY_DIR}/ut_resources.qrc)

# 源码文件（按类分目标编译，保证 lcov 按目标隔离）
set(SETTINGS_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/settings.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/pathsettintwgt.cpp
)
set(UTILS_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/basepub/load_libs.c
)
set(URLINFO_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/basepub/load_libs.c
)
set(CONFIG_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/config.cpp
)
set(EVENTLOG_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/eventlogutils.cpp
)

# ---------------------------------------------------------------------------
# 公共目标辅助函数
# ---------------------------------------------------------------------------
function(add_ut_target TARGET_NAME)
    cmake_parse_arguments(UT "" "" "SOURCES;QT_LIBS;DTK_LIBS;EXTRA_LIBS" ${ARGN})
    add_executable(${TARGET_NAME} ${UT_SOURCES} ${STUB_SHADOW})
    set_target_properties(${TARGET_NAME} PROPERTIES AUTOMOC ON AUTORCC ON)
    target_include_directories(${TARGET_NAME} PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME}
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        Qt${QT_VERSION}::Core
        ${UT_QT_LIBS}
        ${UT_DTK_LIBS}
        ${UT_EXTRA_LIBS}
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(${TARGET_NAME} PRIVATE gcov)
    endif()
    gtest_discover_tests(${TARGET_NAME} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    message(STATUS "UT: ${TARGET_NAME} configured")
endfunction()

# ---------------------------------------------------------------------------
# test_settings：Settings / CustemBackend / KeySequenceEdit（GUI 混合，offscreen）
# ---------------------------------------------------------------------------
add_ut_target(test_settings
    SOURCES test_settings.cpp ${SETTINGS_SOURCES} ${UT_RESOURCES_QRC}
    QT_LIBS Qt${QT_VERSION}::Gui Qt${QT_VERSION}::Widgets Qt${QT_VERSION}::Svg Qt${QT_VERSION}::Core5Compat Qt${QT_VERSION}::Test
    DTK_LIBS Dtk${QT_VERSION}::Widget Dtk${QT_VERSION}::Core
)

# ---------------------------------------------------------------------------
# test_config：Config（DConfig stub + iconv 检测）
# ---------------------------------------------------------------------------
add_ut_target(test_config
    SOURCES test_config.cpp ${CONFIG_SOURCES}
    QT_LIBS Qt${QT_VERSION}::Gui
    DTK_LIBS Dtk${QT_VERSION}::Core
)

# ---------------------------------------------------------------------------
# test_utils：Utils（含 Settings 依赖，GUI offscreen）
# ---------------------------------------------------------------------------
add_ut_target(test_utils
    SOURCES test_utils.cpp ${UTILS_SOURCES} ${SETTINGS_SOURCES} ${UT_RESOURCES_QRC}
    QT_LIBS Qt${QT_VERSION}::Gui Qt${QT_VERSION}::Widgets Qt${QT_VERSION}::Svg Qt${QT_VERSION}::DBus Qt${QT_VERSION}::Core5Compat Qt${QT_VERSION}::Test
    DTK_LIBS Dtk${QT_VERSION}::Widget Dtk${QT_VERSION}::Core
    EXTRA_LIBS KF6::Codecs
)

# ---------------------------------------------------------------------------
# test_urlinfo：UrlInfo（header-only，Core 即可）
# ---------------------------------------------------------------------------
add_ut_target(test_urlinfo
    SOURCES test_urlinfo.cpp ${URLINFO_SOURCES}
    QT_LIBS Qt${QT_VERSION}::Gui Qt${QT_VERSION}::Widgets Qt${QT_VERSION}::Svg Qt${QT_VERSION}::DBus Qt${QT_VERSION}::Core5Compat
    DTK_LIBS Dtk${QT_VERSION}::Widget Dtk${QT_VERSION}::Core
    EXTRA_LIBS KF6::Codecs
)

# ---------------------------------------------------------------------------
# test_eventlog：Eventlogutils（QLibrary stub，Core）
# ---------------------------------------------------------------------------
add_ut_target(test_eventlog
    SOURCES test_eventlog.cpp ${EVENTLOG_SOURCES}
    QT_LIBS
    DTK_LIBS
)

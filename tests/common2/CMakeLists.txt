# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# 模块 common2：src/common 第 2 部分 + src/encodes + src/basepub
# 类：TextFileSaver / FileLoadThread / PerformanceMonitor / CSyntaxHighlighter /
#     SaveFileInterface(dbusinterface) / IflytekAiAssistant / DetectCode / load_libs

enable_language(C)  # 编译 src/basepub/load_libs.c

# C 源码同样启用 gcov 插桩（顶层仅设置了 CXX 标志）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -O0")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus Concurrent Test Core5Compat)
find_package(Dtk6 REQUIRED COMPONENTS Core Widget)
find_package(KF6SyntaxHighlighting REQUIRED)
find_package(ICU COMPONENTS i18n uc REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(chardet REQUIRED chardet)
pkg_check_modules(uchardet REQUIRED uchardet)

set(CMAKE_AUTOMOC ON)

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")
set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

# 被测源码（含传递依赖：detectcode -> config；text_file_saver/fileloadthread -> detectcode）
# 注意：utils.cpp 为重 GUI 模块（DSettings/DMessageManager 等），不入目标；
#       其唯一被引用符号 Utils::isMemorySufficientForOperation 由测试文件本地定义并 stub。
set(COMMON2_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/text_file_saver.cpp
    ${CMAKE_SOURCE_DIR}/src/common/fileloadthread.cpp
    ${CMAKE_SOURCE_DIR}/src/common/performancemonitor.cpp
    ${CMAKE_SOURCE_DIR}/src/common/CSyntaxHighlighter.cpp
    ${CMAKE_SOURCE_DIR}/src/common/dbusinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/common/iflytek_ai_assistant.cpp
    ${CMAKE_SOURCE_DIR}/src/common/config.cpp
    ${CMAKE_SOURCE_DIR}/src/encodes/detectcode.cpp
    ${CMAKE_SOURCE_DIR}/src/basepub/load_libs.c
)

add_executable(test_common2
    ${TEST_SOURCES}
    ${COMMON2_SOURCES}
    ${STUB_SHADOW}
)

target_include_directories(test_common2
    PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/3rdparty/stub
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/encodes
    ${CMAKE_SOURCE_DIR}/src/basepub
    ${chardet_INCLUDE_DIRS}
    ${uchardet_INCLUDE_DIRS}
    ${ICU_INCLUDE_DIRS}
)

target_link_libraries(test_common2
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
    Qt6::Concurrent
    Qt6::Test
    Qt6::Core5Compat
    Dtk6::Core
    Dtk6::Widget
    KF6::SyntaxHighlighting
    ${ICU_LIBRARIES}
    ${chardet_LIBRARIES}
    ${uchardet_LIBRARIES}
    pthread
    dl
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(test_common2 PRIVATE gcov)
endif()

gtest_discover_tests(test_common2 PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

message(STATUS "UT: test_common2 configured with ${TEST_SOURCES}")

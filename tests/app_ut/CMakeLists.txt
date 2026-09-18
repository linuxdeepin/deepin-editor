# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 模块 app_ut：EditorApplication（src/editorapplication.cpp，DApplication 子类）
#   - test_editorapplication.cpp : ctor/dtor/handleQuitAction/notify/pressSpace
#     （editorapplication.cpp 全部 7 个 FN：ctor、D2/D0 双析构记录、handleQuitAction、
#     notify、pressSpace 及其 80ms singleShot lambda）
#
# 链接策略：复用 startmanager 模块的全量源码静态库 startmanager_ut_src
#（src/**.cpp 全量 + AUTOMOC/AUTORCC + deepin-editor.qrc）。
# EditorApplication 本身即进程级 QApplication 对象：本目标独立可执行文件内
# 以它作为唯一 QApplication 实例构造（offscreen），不再另建 QCoreApplication。
# 运行期拦截（沿用 widgets_ut 已验证矩阵）：
#   - DBus：QDBusConnection::systemBus/sessionBus → 伪连接 + callWithArgumentList 桩
#     + unregisterService 桩（~StartManager 析构路径）
#   - IflytekAiAssistant::instance/checkAiExists → 伪对象/空操作
#     （StartManager 构造链的真实 iflytek 查询被隔离）
# 资源说明：qrc 静态库符号丢弃问题——目标直接编入 deepin-editor.qrc（AUTORCC），
# dtor 用例构造真实 StartManager → Settings::instance() 需要 qrc settings.json。

set(QT_VERSION 6)
set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_editorapplication
    test_editorapplication.cpp
    ${CMAKE_SOURCE_DIR}/src/deepin-editor.qrc
    ${STUB_SHADOW}
)
set_target_properties(test_editorapplication PROPERTIES AUTORCC ON)
target_include_directories(test_editorapplication PRIVATE
    "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub"
)
# 访问 protected（handleQuitAction/notify）与 private（pressSpace）用于直接驱动；
# 访问控制不影响 ABI 布局，与库侧 AUTOMOC 产物 ODR 兼容
target_compile_options(test_editorapplication PRIVATE -fno-access-control)
target_link_options(test_editorapplication PRIVATE -rdynamic)
target_link_libraries(test_editorapplication PRIVATE
    startmanager_ut_src
    GTest::gtest
    GTest::gtest_main
    Qt${QT_VERSION}::Test
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(test_editorapplication PRIVATE gcov)
endif()
gtest_discover_tests(test_editorapplication PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

message(STATUS "UT: app_ut module configured (test_editorapplication, links startmanager_ut_src)")

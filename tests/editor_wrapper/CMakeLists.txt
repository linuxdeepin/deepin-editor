# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 模块 editor_wrapper（批次 B8）：src/editor/editwrapper.h/.cpp（EditWrapper 单类）
#
# 链接策略：复用 startmanager 批次的全量源码静态库 startmanager_ut_src
#（src/**.cpp 全量 + AUTOMOC/AUTORCC + deepin-editor.qrc，TextEdit/BottomBar/
#  WarningNotices/LeftAreaTextEdit/Settings/Utils/markdown 模块全部真实链接），
# 本模块只编译测试 TU。EditWrapper 真实构造（offscreen QApplication 已验证可行），
# 外围重依赖在运行期拦截：
#   - Window / Tabbar：测试从不构造实例，以伪指针（reinterpret_cast 自真实 QWidget）
#     传入，被调非虚成员全部由 stub_ext 记录桩拦截（同 editor_undo/startmanager 模式）；
#   - DRecentManager::addItem：拦截，避免写真实 recently-used；
#   - DDialog::exec / QDialog::exec / QFileDialog::selectedFiles / DFileDialog::
#     getComboBoxValue：拦截，避免 offscreen 模态阻塞；
#   - DMessageManager::sendMessage / Utils::sendFloatMessageFixedFont：拦截计数，
#     替代对浮动消息控件的脆弱断言。
# 文件系统隔离：XDG_CONFIG_HOME / XDG_DATA_HOME 重定向 QTemporaryDir，
# 草稿/备份目录（AppDataLocation 下 blank-files / backup-files）随之可控。
# 访问私有成员（m_pWaringNotices/m_sCurEncode 等）用于状态断言：
#   -fno-access-control 不影响 ABI 布局，与库侧 AUTOMOC 产物 ODR 兼容。

set(QT_VERSION 6)
set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

# QSignalSpy 需要 QtTest；根 tests/CMakeLists.txt 未包含该组件，此处补找
find_package(Qt6 REQUIRED COMPONENTS Test)

# 资源说明：startmanager_ut_src 为静态库，其 deepin-editor.qrc 目标文件因无被引用
# 符号不会被链接器拉入（qrc 自注册失效），:/resources/settings.json 等资源缺失会
# 导致 Settings 空指针崩溃。故在本目标直接编入同一 qrc（AUTORCC）。
add_executable(test_editwrapper
    test_editwrapper.cpp
    ${CMAKE_SOURCE_DIR}/src/deepin-editor.qrc
    ${STUB_SHADOW}
)

set_target_properties(test_editwrapper PROPERTIES AUTORCC ON)

target_include_directories(test_editwrapper PRIVATE
    "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub"
)

# 访问控制不影响 ABI 布局；-rdynamic 导出符号供 stub_ext 符号定位兜底
target_compile_options(test_editwrapper PRIVATE -fno-access-control)
target_link_options(test_editwrapper PRIVATE -rdynamic)

target_link_libraries(test_editwrapper PRIVATE
    startmanager_ut_src
    GTest::gtest
    GTest::gtest_main
    Qt${QT_VERSION}::Test
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(test_editwrapper PRIVATE gcov)
endif()

gtest_discover_tests(test_editwrapper PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

message(STATUS "UT: test_editwrapper configured (B8, links startmanager_ut_src)")

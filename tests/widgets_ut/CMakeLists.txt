# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 模块 widgets_ut（批次 B11）：src/widgets/ 六文件
#   - test_ddropdownmenu.cpp  : DDropdownMenu（QFrame 下拉菜单）
#   - test_colorselectwdg.cpp : ColorLabel + ColorSelectWdg（右键菜单颜色标记）
#   - test_pathsettintwgt.cpp : PathSettingWgt（保存路径设置控件）
#   - test_bottombar.cpp      : BottomBar（状态栏）
#   - test_window.cpp         : Window（主窗口，真实 offscreen 构造）
#
# 链接策略：复用 startmanager 批次的全量源码静态库 startmanager_ut_src
#（src/**.cpp 全量 + AUTOMOC/AUTORCC + deepin-editor.qrc）。
# 真实构造（offscreen QApplication 已在 B8 验证可行），外围重依赖运行期拦截：
#   - DBus：QDBusConnection::systemBus/sessionBus → 伪连接名，禁止真实总线；
#   - 模态对话框：DDialog::exec / QDialog::exec / QFileDialog::selectedFiles /
#     DFileDialog::getComboBoxValue 拦截，避免 offscreen 模态阻塞；
#   - 浮动消息：DMessageManager::sendMessage 计数桩；
#   - 子进程：QProcess::startDetached 拦截（displayShortcuts）；
#   - 文件系统：XDG_CONFIG_HOME/XDG_DATA_HOME → QTemporaryDir。
# 访问私有成员（m_wrappers/m_pendingTabs 等）用于状态注入与断言：
#   -fno-access-control 不影响 ABI 布局，与库侧 AUTOMOC 产物 ODR 兼容。
# 资源说明：qrc 静态库符号丢弃问题——目标直接编入 deepin-editor.qrc（AUTORCC）。

set(QT_VERSION 6)
set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

find_package(Qt6 REQUIRED COMPONENTS Test)

function(add_widgets_ut_target NAME)
    add_executable(${NAME}
        ${NAME}.cpp
        ${CMAKE_SOURCE_DIR}/src/deepin-editor.qrc
        ${STUB_SHADOW}
    )
    set_target_properties(${NAME} PROPERTIES AUTORCC ON)
    target_include_directories(${NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub"
    )
    target_compile_options(${NAME} PRIVATE -fno-access-control)
    target_link_options(${NAME} PRIVATE -rdynamic)
    target_link_libraries(${NAME} PRIVATE
        startmanager_ut_src
        GTest::gtest
        GTest::gtest_main
        Qt${QT_VERSION}::Test
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(${NAME} PRIVATE gcov)
    endif()
    gtest_discover_tests(${NAME} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

add_widgets_ut_target(test_ddropdownmenu)
add_widgets_ut_target(test_colorselectwdg)
add_widgets_ut_target(test_pathsettintwgt)
add_widgets_ut_target(test_bottombar)
add_widgets_ut_target(test_window)
# TextEdit×Window 集成（dtextedit.cpp 最后 2 个 FNDA:0：dragEnterEvent/pinchTriggered
# 需真实 Window 父链，editor_core 禁真实 Window，按论证落位本模块）
add_widgets_ut_target(test_dtextedit_window_integration)

message(STATUS "UT: widgets_ut module configured (B11, links startmanager_ut_src)")

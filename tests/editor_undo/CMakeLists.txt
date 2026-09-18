# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 模块 editor_undo（批次 B5）：src/editor 撤销/重做命令类（QUndoCommand 子类）+
# UndoList 组合命令。共 14 个类，逐类独立可执行文件，共享一个静态核心库
#（被测 9 个源文件 + 链接接缝 editor_undo_seam.cpp）。
#
# 链接接缝说明（详见 editor_undo_seam.h）：TextEdit / EditWrapper / Window /
# BottomBar 的真实实现不参与链接；本目录所有目标一律不开 AUTOMOC
#（Q_OBJECT 类不实例化元对象特性，空壳定义由 editor_undo_seam.cpp 提供），
# 因此此处显式 set(CMAKE_AUTOMOC OFF) 防御上游目录属性渗透。

set(CMAKE_AUTOMOC OFF)

set(EDITOR_DIR "${CMAKE_SOURCE_DIR}/src/editor")
set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

# ---- 静态核心库：被测源码 + 链接接缝（含传递头文件闭包的 include 目录） ----
add_library(editor_undo_core STATIC
    ${EDITOR_DIR}/inserttextundocommand.cpp
    ${EDITOR_DIR}/deletebackcommond.cpp
    ${EDITOR_DIR}/deletetextundocommand.cpp
    ${EDITOR_DIR}/indenttextcommond.cpp
    ${EDITOR_DIR}/insertblockbytextcommond.cpp
    ${EDITOR_DIR}/changemarkcommand.cpp
    ${EDITOR_DIR}/endlineformatcommond.cpp
    ${EDITOR_DIR}/replaceallcommond.cpp
    ${EDITOR_DIR}/undolist.cpp
    editor_undo_seam.cpp
)

target_include_directories(editor_undo_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/editor"
    "${CMAKE_SOURCE_DIR}/src/widgets"
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/editor/markdown"
)

target_link_libraries(editor_undo_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
    Qt6::PrintSupport
    Qt6::Core5Compat
    Dtk6::Core
    Dtk6::Widget
    KF6::SyntaxHighlighting
)

# ---- 逐类测试可执行文件 ----
function(eu_add_test name)
    add_executable(test_${name} test_${name}.cpp ${STUB_SHADOW})
    target_include_directories(test_${name} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub"
    )
    target_link_libraries(test_${name} PRIVATE
        editor_undo_core
        GTest::gtest
        GTest::gtest_main
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(test_${name} PRIVATE gcov)
    endif()
    gtest_discover_tests(test_${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    message(STATUS "UT: test_${name} configured")
endfunction()

eu_add_test(inserttextundocommand)
eu_add_test(midbuttoninserttextundocommand)
eu_add_test(draginserttextundocommand)
eu_add_test(deletebackcommand)
eu_add_test(deletebackaltcommand)
eu_add_test(deletetextundocommand)
eu_add_test(deletetextundocommand2)
eu_add_test(indenttextcommand)
eu_add_test(unindenttextcommand)
eu_add_test(insertblockbytextcommand)
eu_add_test(changemarkcommand)
eu_add_test(endlineformartcommand)
eu_add_test(replaceallcommand)
eu_add_test(undolist)

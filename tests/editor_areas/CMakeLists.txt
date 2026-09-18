cmake_minimum_required(VERSION 3.16)

# B6 模块：editor_areas（src/editor 区域/辅助组件类）
# 类：LeftAreaTextEdit / LineNumberArea / BookMarkWidget / CodeFlodArea /
#     ShowFlodCodeWidget / FlashTween / MarkOperation+MarkReplaceInfo(dtextedit.h 结构体)
#
# 策略（沿用 B4）：全量编入 src/ 为共享静态库（排除入口 main 与冲突源 encoding.cpp），
# 保证 dtextedit.cpp（含 MarkOperation 静态转换函数）等符号齐全；
# 运行期对 TextEdit 的依赖用 stub_ext 拦截（LeftAreaTextEdit 仅持有指针，
# 经 fake 指针 + stub 喂数据），QWidget 测试在 QT_QPA_PLATFORM=offscreen 下构造。

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# environments.h（VERSION 宏，部分源码需要）
configure_file(
    "${CMAKE_SOURCE_DIR}/src/environments.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/environments.h"
    @ONLY
)

# ---- 共享静态库：项目源码（多个 test target 复用，避免重复编译） ----
file(GLOB_RECURSE EA_ALL_SRC
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/basepub/*.c")
list(REMOVE_ITEM EA_ALL_SRC
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/encoding.cpp")

add_library(editor_areas_ut_src STATIC
    ${EA_ALL_SRC}
    "${CMAKE_SOURCE_DIR}/src/deepin-editor.qrc")

target_include_directories(editor_areas_ut_src PUBLIC
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/controls"
    "${CMAKE_SOURCE_DIR}/src/editor"
    "${CMAKE_SOURCE_DIR}/src/editor/markdown"
    "${CMAKE_SOURCE_DIR}/src/widgets"
    "${CMAKE_SOURCE_DIR}/src/dbus"
    "${CMAKE_SOURCE_DIR}/src/thememodule"
    "${CMAKE_SOURCE_DIR}/src/encodes"
    "${CMAKE_SOURCE_DIR}/src/basepub"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(editor_areas_ut_src PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Xml
    Qt6::Svg
    Qt6::Concurrent
    Qt6::PrintSupport
    Qt6::DBus
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    Qt6::OpenGLWidgets
    Qt6::Core5Compat
    Dtk6::Widget
    Dtk6::Core
    KF6::Codecs
    KF6::SyntaxHighlighting
    ICU::i18n
    ICU::uc
    ${chardet_LIBRARIES}
    uchardet
    pthread
    dl
    m
)

# stub-ext 运行时依赖
set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

# 代码覆盖率（Debug，沿用 tests 顶层全局 flags，此处仅链 gcov）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(editor_areas_ut_src PUBLIC gcov)
endif()

# QSignalSpy / QApplication::sendEvent 需要 Qt6::Test（顶层未包含该组件）
find_package(Qt6 COMPONENTS Test REQUIRED)

# ---- 逐类 test target ----
function(ea_add_test name)
    add_executable(test_${name}
        test_${name}.cpp
        ${STUB_SHADOW})
    target_include_directories(test_${name} PRIVATE
        "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub")
    # 访问 protected（paintEvent/sizeHint）与 private（FlashTween 槽/成员、
    # LeftAreaTextEdit::m_pTextEdit 等）用于直接驱动与状态断言；
    # 访问控制不影响 ABI 布局，与库侧 AUTOMOC 产物 ODR 兼容
    target_compile_options(test_${name} PRIVATE -fno-access-control)
    target_link_libraries(test_${name} PRIVATE
        editor_areas_ut_src
        GTest::gtest
        GTest::gtest_main
        Qt6::Test)
    gtest_discover_tests(test_${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    message(STATUS "UT: test_${name} configured")
endfunction()

ea_add_test(flashtween)
ea_add_test(leftareatextedit)
ea_add_test(linenumberarea)
ea_add_test(bookmarkwidget)
ea_add_test(codeflodarea)
ea_add_test(showflodcodewidget)
ea_add_test(markoperation)

message(STATUS "UT: editor_areas module configured")

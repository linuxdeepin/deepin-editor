cmake_minimum_required(VERSION 3.16)

# B7 模块：editor_core（TextEdit 单类，src/editor/dtextedit.h/.cpp，9589 行，全项目最大类）
#
# 策略（沿用 B4/B6 已验证方案）：全量编入 src/ 为共享静态库（排除入口 main.cpp
# 与冲突源 encoding.cpp），保证 TextEdit 的真实依赖链（LeftAreaTextEdit/
# ShowFlodCodeWidget/撤销命令族/Settings/Utils/iflytek）符号齐全；
# 运行期对 EditWrapper/Window/BottomBar 的调用用 stub_ext 拦截
# （fake 指针 + 桩函数喂返回值），绝不真实构造 Window。
# QWidget 测试在 QT_QPA_PLATFORM=offscreen 下构造（Wave1 已验证）；
# 测试目标开启 -fno-access-control 访问 private/protected 成员（B6 先例）。

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 源码侧编译宏（与根 CMakeLists.txt 保持一致）
add_definitions(-DKF5_HIGHLIGHT_PATH="/usr/share/deepin-editor/highlighting")

# environments.h（VERSION 宏，部分源码需要）
configure_file(
    "${CMAKE_SOURCE_DIR}/src/environments.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/environments.h"
    @ONLY
)

# ---- 共享静态库：项目源码（多个 test target 复用，避免重复编译） ----
file(GLOB_RECURSE EC_ALL_SRC
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/basepub/*.c")
list(REMOVE_ITEM EC_ALL_SRC
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/encoding.cpp")

add_library(editor_core_ut_src STATIC
    ${EC_ALL_SRC}
    "${CMAKE_SOURCE_DIR}/src/deepin-editor.qrc")

target_include_directories(editor_core_ut_src PUBLIC
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

target_link_libraries(editor_core_ut_src PUBLIC
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
    target_link_libraries(editor_core_ut_src PUBLIC gcov)
endif()

# QSignalSpy / QApplication::sendEvent 需要 Qt6::Test（顶层未包含该组件）
find_package(Qt6 COMPONENTS Test REQUIRED)

# ---- 逐功能组 test target（TextEdit 按方法族拆分 6 个文件） ----
function(ec_add_test name)
    add_executable(test_${name}
        test_${name}.cpp
        ${STUB_SHADOW})
    target_include_directories(test_${name} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub")
    # 访问 protected（事件处理器）与 private（m_wrapper/m_markOperations 等）
    # 用于直接驱动与状态断言；访问控制不影响 ABI 布局，与库侧 AUTOMOC 产物 ODR 兼容
    target_compile_options(test_${name} PRIVATE -fno-access-control)
    target_link_libraries(test_${name} PRIVATE
        editor_core_ut_src
        GTest::gtest
        GTest::gtest_main
        Qt6::Test)
    gtest_discover_tests(test_${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    message(STATUS "UT: test_${name} configured")
endfunction()

ec_add_test(dtextedit_cursor)
ec_add_test(dtextedit_edit)
ec_add_test(dtextedit_mark)
ec_add_test(dtextedit_find)
ec_add_test(dtextedit_event)
ec_add_test(dtextedit_misc)

message(STATUS "UT: editor_core module configured")

cmake_minimum_required(VERSION 3.16)

# B4 模块：startmanager（StartManager / FileTabInfo）
# 策略：全量编入 src/（与旧 tests 相同方式，排除入口 main 与冲突源），
# 保证 startmanager.cpp 的 Window/EditWrapper/Settings/Utils 等符号齐全；
# 运行期外部交互（DBus/Settings 单例/QStandardPaths/屏幕几何/Window 构造）
# 全部由 stub_ext 拦截，文件系统隔离到 QTemporaryDir。

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_VERSION 6)

# environments.h（仅 VERSION 宏，editorapplication.cpp 需要）
configure_file(
    "${CMAKE_SOURCE_DIR}/src/environments.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/environments.h"
    @ONLY
)

# ---- 共享静态库：项目源码（两个 test target 复用，避免重复编译） ----
file(GLOB_RECURSE SM_ALL_SRC
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/basepub/*.c")
list(REMOVE_ITEM SM_ALL_SRC
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/encoding.cpp")

add_library(startmanager_ut_src STATIC
    ${SM_ALL_SRC}
    "${CMAKE_SOURCE_DIR}/src/deepin-editor.qrc")

target_include_directories(startmanager_ut_src PUBLIC
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

target_link_libraries(startmanager_ut_src PUBLIC
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
    target_link_libraries(startmanager_ut_src PUBLIC gcov)
endif()

# ---- test_startmanager ----
add_executable(test_startmanager
    test_startmanager.cpp
    ${STUB_SHADOW})
target_include_directories(test_startmanager PRIVATE
    "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub")
# 访问私有成员（m_windows/m_qlistTemFile 等）用于状态注入与断言；
# 访问控制不影响 ABI 布局，与库侧 AUTOMOC 产物 ODR 兼容
target_compile_options(test_startmanager PRIVATE -fno-access-control)
# 导出符号到动态符号表：dlsym 定位 Window 构造符号（用于 createWindow 真实执行用例）
target_link_options(test_startmanager PRIVATE -rdynamic)
target_link_libraries(test_startmanager PRIVATE
    startmanager_ut_src
    GTest::gtest
    GTest::gtest_main)

# ---- test_filetabinfo ----
add_executable(test_filetabinfo
    test_filetabinfo.cpp
    ${STUB_SHADOW})
target_include_directories(test_filetabinfo PRIVATE
    "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub")
target_link_libraries(test_filetabinfo PRIVATE
    startmanager_ut_src
    GTest::gtest
    GTest::gtest_main)

# ---- test_startmanager_drag ----
# createWindow 内 DTabBar::dragStarted/dragEnd 私接 lambda（startmanager.cpp:773/777）：
# 需 QApplication + 真实 Window/Tabbar（QCoreApplication 语境不可达），独立可执行文件。
# 真实 Settings::instance() 依赖 qrc settings.json——静态库符号丢弃问题，qrc 直编目标
add_executable(test_startmanager_drag
    test_startmanager_drag.cpp
    ${CMAKE_SOURCE_DIR}/src/deepin-editor.qrc
    ${STUB_SHADOW})
set_target_properties(test_startmanager_drag PROPERTIES AUTORCC ON)
target_include_directories(test_startmanager_drag PRIVATE
    "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub")
# 白盒断言 m_bIsTagDragging/m_DelayTimer/m_windows；访问控制不影响 ABI 布局
target_compile_options(test_startmanager_drag PRIVATE -fno-access-control)
target_link_libraries(test_startmanager_drag PRIVATE
    startmanager_ut_src
    GTest::gtest
    GTest::gtest_main)

gtest_discover_tests(test_startmanager PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
gtest_discover_tests(test_filetabinfo PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
gtest_discover_tests(test_startmanager_drag PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

message(STATUS "UT: startmanager module configured (test_startmanager, test_filetabinfo, test_startmanager_drag)")

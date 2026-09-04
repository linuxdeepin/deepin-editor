cmake_minimum_required(VERSION 3.16)

# B12 模块：controls_ut（src/controls/：FindBar / ReplaceBar / JumpLineBar / LineBar /
#   SettingsDialog(GenerateSettingTranslate) / WarningNotices / FontItemDelegate /
#   Tabbar / ToolBar，共 9 文件）
# 策略：复用 startmanager 子目录的全量源码静态库 startmanager_ut_src（含 AUTOMOC
# 产物与全部传递依赖），本模块只编译测试 TU。
# 运行期外部交互全部由 stub_ext 拦截：
#   - Utils::localDataPath / Utils::isBackupFile / Utils::isWayland → 受控假值
#   - Window 非虚成员（wrapper/removeWrapper/addTabWithWrapper/focusActiveEditor/
#     setChildrenFocus）→ 记录桩（测试宿主为普通 QWidget，绝不构造真实 Window）
#   - StartManager::instance / createWindowFromWrapper → 记录桩（避免真实建窗）
#   - QMenu::exec（Tabbar 右键菜单）→ 拦截返回，避免 offscreen 模态阻塞
#   - QFontMetrics::horizontalAdvance → 超长路径换行分支受控触发
# 文件系统隔离：XDG_CONFIG_HOME / XDG_DATA_HOME 重定向 QTemporaryDir。
# QT_QPA_PLATFORM=offscreen 无头运行。

set(QT_VERSION 6)

# QSignalSpy 需要 Qt6 Test 组件
find_package(Qt6 REQUIRED COMPONENTS Test)

set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")

add_executable(test_controls_ut
    ${TEST_SOURCES}
    ${STUB_SHADOW})

# 资源说明：startmanager_ut_src 为静态库，qrc 目标文件未被引用符号拉入时自注册
# 失效；编入同一 qrc 保证 Settings 资源可用（AUTORCC）。
set_target_properties(test_controls_ut PROPERTIES AUTORCC ON)
target_sources(test_controls_ut PRIVATE "${CMAKE_SOURCE_DIR}/src/deepin-editor.qrc")

target_include_directories(test_controls_ut
    PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/3rdparty/stub
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/controls)

# 访问 protected/private 成员（keyPressEvent/paint/eventFilter、m_tabPaths 等）；
# 不改变 ABI 布局，与库侧 AUTOMOC 产物 ODR 兼容
target_compile_options(test_controls_ut PRIVATE -fno-access-control)
# 导出符号到动态符号表（stub_ext 函数入口 patch）
target_link_options(test_controls_ut PRIVATE -rdynamic)

target_link_libraries(test_controls_ut PRIVATE
    startmanager_ut_src
    GTest::gtest
    GTest::gtest_main
    Qt${QT_VERSION}::Test)  # QSignalSpy

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(test_controls_ut PRIVATE gcov)
endif()

gtest_discover_tests(test_controls_ut PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

message(STATUS "UT: controls_ut configured with ${TEST_SOURCES}")

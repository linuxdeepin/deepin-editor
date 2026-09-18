cmake_minimum_required(VERSION 3.16)

# B9 模块：thememodule（ThemeListModel / ThemeListView / ThemeItemDelegate / ThemePanel）
# 策略：复用 startmanager 子目录的全量源码静态库 startmanager_ut_src（含 AUTOMOC 产物，
# themepanel.cpp 相对包含的真实 ../widgets/window.h 无法用 -I 遮蔽，故链接真实源码库；
# 运行期绝不实例化 Window：popup/hide 仅经 m_window->geometry()（QWidget 单继承链
# 非虚方法）读取父窗口几何，测试传普通 QWidget 父对象）。
# 运行期外部交互全部由 stub_ext 拦截：
#   - QDir::entryInfoList（主题目录扫描）与 Utils::getThemeMapFromPath（JSON 解析）
#     → 受控假数据，不触真实文件系统
#   - QApplication::sendEvent（LayoutRequest 计数）、QPainter::fillPath（paintEvent 落笔断言）
#   - QPropertyAnimation 动画真实运行（offscreen 定时器驱动，事件循环收尾）
# QT_QPA_PLATFORM=offscreen 无头运行。

set(QT_VERSION 6)

# QSignalSpy 需要 Qt6 Test 组件
find_package(Qt6 REQUIRED COMPONENTS Test)

set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")

add_executable(test_thememodule
    ${TEST_SOURCES}
    ${STUB_SHADOW})

target_include_directories(test_thememodule
    PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/3rdparty/stub
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/thememodule)

# 访问 protected/private 成员（paint/sizeHint/eventFilter/selectionChanged/paintEvent、
# m_themes/m_themeView/m_themeModel/m_frameColor 等）；不改变 ABI 布局，与库侧 AUTOMOC 产物 ODR 兼容
target_compile_options(test_thememodule PRIVATE -fno-access-control)
# 导出符号到动态符号表（stub_ext 函数入口 patch）
target_link_options(test_thememodule PRIVATE -rdynamic)

target_link_libraries(test_thememodule PRIVATE
    startmanager_ut_src
    GTest::gtest
    GTest::gtest_main
    Qt6::Test)  # QSignalSpy

# 覆盖率库（Debug 模式下）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(test_thememodule PRIVATE gcov)
endif()

gtest_discover_tests(test_thememodule PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

message(STATUS "UT: thememodule configured with ${TEST_SOURCES}")

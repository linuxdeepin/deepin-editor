cmake_minimum_required(VERSION 3.16)

# B10 模块：daemon_ut（daemon/src 的 DBus/PolicyKitHelper/Utils + src/dbus 生成接口与 D-Bus 类型）
#
# 编译策略（不修改任何项目源码）：
# 1) daemon 为 Qt5 qmake 工程：
#    - polkit-qt5-1 dev 头在本环境不可用 → shim/polkit-qt5-1 头文件转发到系统
#      polkit-qt6-1（同 PolkitQt1 命名空间、API 兼容），链接 libpolkit-qt6-core-1；
#    - daemon/src/dbus.cpp 使用 Qt6 已移除的 QTextStream::setCodec → 仅对该翻译单元以
#      预处理宏 setCodec(a)=setEncoding(QStringConverter::Utf8) 做编译期映射（UT 编译垫片，
#      测试只使用 UTF-8 encoding 入参，语义一致）。
# 2) 运行期外部交互全部 stub_ext 拦截，零真实 D-Bus / 零 polkit 授权调用：
#    - PolkitQt1::Authority::instance / checkAuthorizationSync（test_policykithelper、test_dbus）；
#    - QDBusConnection::call + QDBusMessage::signature + QDBusAbstractInterface::asyncCallWithArgumentList
#      （生成接口的属性 Get/Set 与方法调用，详见 test_comdeepindededaemondockinterface.cpp 头注释）；
#    - 文件系统隔离到 QTemporaryDir。

set(CMAKE_AUTOMOC ON)

set(QT_VERSION 6)

# QSignalSpy（接口测试信号断言）；顶层 find_package 未含 Test 组件，本模块内补齐
find_package(Qt${QT_VERSION} REQUIRED COMPONENTS Test)

# polkit-qt6 core（经 shim 头编译 daemon/src/policykithelper.cpp 并解析其符号）
find_library(POLKIT_QT6_CORE_LIB polkit-qt6-core-1 REQUIRED)

# ---- 共享静态库：被测源码（各 test target 复用，避免重复编译） ----
set(DAEMON_UT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/ut_dbus_compat.cpp"
    "${CMAKE_SOURCE_DIR}/daemon/src/policykithelper.cpp"
    "${CMAKE_SOURCE_DIR}/daemon/src/utils.cpp"
    "${CMAKE_SOURCE_DIR}/src/dbus/com_deepin_dde_daemon_dock.cpp"
    "${CMAKE_SOURCE_DIR}/src/dbus/com_deepin_dde_daemon_dock_entry.cpp"
    "${CMAKE_SOURCE_DIR}/src/dbus/types/windowinfomap.cpp"
    "${CMAKE_SOURCE_DIR}/src/dbus/types/windowlist.cpp")

add_library(daemon_ut_src STATIC ${DAEMON_UT_SRC})

target_include_directories(daemon_ut_src PUBLIC
    "${CMAKE_SOURCE_DIR}/daemon/src"
    "${CMAKE_SOURCE_DIR}/src/dbus"
    # polkit-qt5-1 → polkit-qt6-1 转发垫片（必须先于系统路径生效）
    "${CMAKE_CURRENT_SOURCE_DIR}/shim"
    "/usr/include/polkit-qt6-1")

target_link_libraries(daemon_ut_src PUBLIC
    Qt${QT_VERSION}::Core
    Qt${QT_VERSION}::DBus
    ${POLKIT_QT6_CORE_LIB})

# 代码覆盖率（Debug，沿用 tests 顶层全局 flags，此处仅链 gcov）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(daemon_ut_src PUBLIC gcov)
endif()

# stub-ext 运行时依赖
set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

# ---- 测试 target（每测试文件一个可执行文件） ----
function(daemon_ut_add_test TARGET_NAME TEST_SOURCE)
    add_executable(${TARGET_NAME} ${TEST_SOURCE} ${STUB_SHADOW})
    target_include_directories(${TARGET_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub")
    target_link_libraries(${TARGET_NAME} PRIVATE
        daemon_ut_src
        GTest::gtest
        GTest::gtest_main)
    gtest_discover_tests(${TARGET_NAME} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

daemon_ut_add_test(test_daemon_utils test_utils.cpp)
daemon_ut_add_test(test_daemon_policykithelper test_policykithelper.cpp)
daemon_ut_add_test(test_daemon_dbus test_dbus.cpp)
daemon_ut_add_test(test_windowinfomap test_windowinfomap.cpp)

# Dock 接口测试：-fno-access-control 用于读写 DockRect 私有字段（x/y/w/h）做精确断言；
# 需 QSignalSpy（QtTest）与 QCoreApplication 自定义 main
add_executable(test_comdeepindededaemondockinterface
    test_comdeepindededaemondockinterface.cpp
    ${STUB_SHADOW})
target_include_directories(test_comdeepindededaemondockinterface PRIVATE
    "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub")
target_compile_options(test_comdeepindededaemondockinterface PRIVATE -fno-access-control)
target_link_libraries(test_comdeepindededaemondockinterface PRIVATE
    daemon_ut_src
    GTest::gtest
    Qt${QT_VERSION}::Test)

# Entry 接口测试：同上（QSignalSpy + 自定义 main）
add_executable(test_comdeepindededaemondockentryinterface
    test_comdeepindededaemondockentryinterface.cpp
    ${STUB_SHADOW})
target_include_directories(test_comdeepindededaemondockentryinterface PRIVATE
    "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub")
target_link_libraries(test_comdeepindededaemondockentryinterface PRIVATE
    daemon_ut_src
    GTest::gtest
    Qt${QT_VERSION}::Test)

message(STATUS "UT: daemon_ut module configured (6 test targets)")

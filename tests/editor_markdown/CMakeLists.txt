cmake_minimum_required(VERSION 3.16)

# deepin-editor tests / editor_markdown 子模块
# 纯逻辑类（imarkdownrenderer/markdownbridge/markdownlogic/scrollsync/themeserializer/viewmodefsm）
# 与 WebEngine 相关类（markdownview/markdownimagehandler）逐类独立可执行文件。
#
# 复用根工程 CMAKE_AUTOMOC=ON / CMAKE_CXX_STANDARD=17 / Debug 覆盖率插桩（tests/CMakeLists.txt）。

set(MARKDOWN_SRC_DIR "${CMAKE_SOURCE_DIR}/src/editor/markdown")
set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

function(md_add_test name)
    add_executable(test_${name} ${ARGN} ${STUB_SHADOW})
    target_include_directories(test_${name} PRIVATE
        "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub"
        "${MARKDOWN_SRC_DIR}"
    )
    target_link_libraries(test_${name} PRIVATE
        GTest::gtest
        GTest::gtest_main
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::WebChannel
        Qt6::WebEngineWidgets
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(test_${name} PRIVATE gcov)
    endif()
    gtest_discover_tests(test_${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    message(STATUS "UT: test_${name} configured")
endfunction()

# 头文件列入 target sources 以触发 AUTOMOC（Q_OBJECT 类）
md_add_test(imarkdownrenderer test_imarkdownrenderer.cpp)
md_add_test(markdownbridge     test_markdownbridge.cpp     "${MARKDOWN_SRC_DIR}/markdownbridge.h")
md_add_test(markdownlogic      test_markdownlogic.cpp)
md_add_test(scrollsync         test_scrollsync.cpp)
md_add_test(themeserializer    test_themeserializer.cpp)
md_add_test(viewmodefsm        test_viewmodefsm.cpp)
md_add_test(markdownview       test_markdownview.cpp       "${MARKDOWN_SRC_DIR}/markdownview.cpp" "${MARKDOWN_SRC_DIR}/markdownview.h" "${MARKDOWN_SRC_DIR}/markdownbridge.h")
# markdownimagehandler 定义在 markdownview.cpp 匿名命名空间：测试 TU 直接 include 该 cpp
md_add_test(markdownimagehandler test_markdownimagehandler.cpp "${MARKDOWN_SRC_DIR}/markdownview.h" "${MARKDOWN_SRC_DIR}/markdownbridge.h")
# RenderThrottle（renderthrottle.h 纯逻辑节流器）：补齐唯一 FNDA:0 私有槽 onTimeout（后沿补发），
# 头文件列入 sources 触发 AUTOMOC（Q_OBJECT）；QSignalSpy/QTest::qWait 需要 Qt6::Test
md_add_test(renderthrottle test_renderthrottle.cpp "${MARKDOWN_SRC_DIR}/renderthrottle.h")

# QSignalSpy（QtTest 模块）仅信号验证类目标链接
find_package(Qt6 COMPONENTS Test)
target_link_libraries(test_markdownbridge PRIVATE Qt6::Test)
target_link_libraries(test_markdownview PRIVATE Qt6::Test)
target_link_libraries(test_renderthrottle PRIVATE Qt6::Test)

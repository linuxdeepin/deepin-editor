# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: CC0-1.0

# UnitTestUtils.cmake - Universal C++ Unit Test CMake Utilities
# Version: 5.0.0

cmake_minimum_required(VERSION 3.16)

set(CPP_STUB_SRC "" CACHE INTERNAL "Stub source files for testing")
set(UT_TEST_CXX_FLAGS "" CACHE INTERNAL "Test-specific CXX flags")

function(add_subdirectory_if_exists dir)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        add_subdirectory(${dir})
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
        message(STATUS "UT: Subdirectory ${dir} exists but has no CMakeLists.txt")
    else()
        message(STATUS "UT: Subdirectory ${dir} does not exist, skipping")
    endif()
endfunction()

function(ut_init_test_environment)
    message(STATUS "UT: Initializing test environment...")
    
    # 根据用户选择的测试框架进行初始化
    if(USE_QT_TEST)
        # Qt Test 框架
        find_package(Qt${QT_VERSION} COMPONENTS Test REQUIRED)
        message(STATUS "UT: Using Qt${QT_VERSION} Test")
    elseif(USE_GTEST)
        # Google Test 框架
        find_package(GTest REQUIRED)
        message(STATUS "UT: Using Google Test")
    elseif(USE_CATCH2)
        # Catch2 框架
        find_package(Catch2 3 REQUIRED)
        message(STATUS "UT: Using Catch2")
    else()
        # 默认使用 Google Test
        find_package(GTest REQUIRED)
        message(STATUS "UT: Using Google Test (default)")
    endif()
    
    # 设置 stub 工具
    ut_setup_test_stubs()
    
    # 设置覆盖率（可选）
    ut_setup_coverage()
    
    message(STATUS "UT: Test environment initialized")
endfunction()

function(ut_setup_test_stubs)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub")
        message(WARNING "UT: stub not found, stub functionality will be limited")
        return()
    endif()
    
    message(STATUS "UT: Setting up test stubs...")
    
    file(GLOB STUB_SRC_FILES
        "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/*.h"
        "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/*.hpp"
        "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/*.cpp"
    )
    
    if(STUB_SRC_FILES)
        set(CPP_STUB_SRC ${STUB_SRC_FILES} CACHE INTERNAL "Stub source files")
        message(STATUS "UT: Found stub files:")
        foreach(stub_file ${STUB_SRC_FILES})
            message(STATUS "    ${stub_file}")
        endforeach()
        
        include_directories(
            "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub"
        )
        message(STATUS "UT: Stub tools configured")
    else()
        message(WARNING "UT: No stub source files found")
    endif()
endfunction()

function(ut_setup_coverage)
    message(STATUS "UT: Setting up code coverage...")
    
    # 覆盖率标志（仅在 Debug 模式下启用）
    if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(TEST_FLAGS "-fno-inline;-fno-access-control;-O0;-fprofile-arcs;-ftest-coverage")
        
        # Address Sanitizer（可选）
        if(ENABLE_ASAN)
            list(APPEND TEST_FLAGS "-fsanitize=address,undefined")
            message(STATUS "UT: ASAN enabled (address,undefined)")
        endif()
        
        set(UT_TEST_CXX_FLAGS ${TEST_FLAGS} CACHE INTERNAL "Test-specific CXX flags")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TEST_FLAGS}" PARENT_SCOPE)
        
        message(STATUS "UT: Coverage configured")
        message(STATUS "  - Test flags: ${TEST_FLAGS}")
    else()
        message(STATUS "UT: Coverage disabled")
    endif()
endfunction()

function(ut_create_test_executable test_name)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs SOURCES HEADERS DEPENDENCIES LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    message(STATUS "UT: Creating test executable: ${test_name}")
    
    set(ALL_SOURCES ${TEST_SOURCES})
    if(TEST_HEADERS)
        list(APPEND ALL_SOURCES ${TEST_HEADERS})
    endif()
    if(CPP_STUB_SRC)
        list(APPEND ALL_SOURCES ${CPP_STUB_SRC})
    endif()
    
    add_executable(${test_name} ${ALL_SOURCES})
    
    # 应用测试标志
    if(UT_TEST_CXX_FLAGS)
        target_compile_options(${test_name} PRIVATE ${UT_TEST_CXX_FLAGS})
        message(STATUS "UT: Applied test flags to ${test_name}: ${UT_TEST_CXX_FLAGS}")
    endif()
    
    # 链接测试框架库
    if(USE_QT_TEST)
        target_link_libraries(${test_name} PRIVATE Qt${QT_VERSION}::Test)
    elseif(USE_GTEST)
        target_link_libraries(${test_name} PRIVATE GTest::gtest GTest::gtest_main)
    elseif(USE_CATCH2)
        target_link_libraries(${test_name} PRIVATE Catch2::Catch2WithMain)
    endif()
    
    # 链接用户指定的库
    if(TEST_LINK_LIBRARIES)
        target_link_libraries(${test_name} PRIVATE ${TEST_LINK_LIBRARIES})
    endif()
    
    # 链接覆盖率库（如果启用）
    if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(${test_name} PRIVATE gcov pthread)
    endif()
    
    # 添加测试
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    message(STATUS "UT: Created test executable: ${test_name}")
endfunction()

message(STATUS "UT: Unit test utilities loaded")

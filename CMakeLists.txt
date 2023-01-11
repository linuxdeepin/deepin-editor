cmake_minimum_required(VERSION 3.7)

project(deepin-editor)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "sw_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee")
endif ()

# 设置软件版本
include_directories(${PROJECT_BINARY_DIR})
configure_file(${PROJECT_SOURCE_DIR}/src/environments.h.in ${PROJECT_BINARY_DIR}/environments.h @ONLY)

add_subdirectory(src)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_subdirectory(tests)
endif()

cmake_minimum_required(VERSION 3.7)

if (NOT DEFINED VERSION)
    set(VERSION 1.2.2)
endif ()

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

add_subdirectory(third)

add_subdirectory(src)

add_subdirectory(test)

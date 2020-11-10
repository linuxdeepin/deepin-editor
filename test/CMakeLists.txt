# Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
#
# Author:     zhangteng <zhangteng@uniontech.com>
#
# Maintainer: zhangteng <zhangteng@uniontech.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
ADD_COMPILE_OPTIONS(-fno-access-control) 
set(APP_QRC "../src/deepin-editor.qrc")

# Find the library
find_package(PkgConfig REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(DtkWidget REQUIRED)
find_package(DtkCore REQUIRED)
find_package(KF5SyntaxHighlighting)
find_package(KF5Codecs)
find_package(DFrameworkdbus REQUIRED)
find_package(Qt5Test REQUIRED)

include_directories(googletest)                           #以下三行添加包含以便测试用例代码的编写
include_directories(googletest/googlemock/include)
include_directories(Qt5PrintSupport)
include_directories(googletest/googletest/include)
add_subdirectory(googletest)                              #添加googletest库
include_directories(src)

set(PROJECT_NAME_TEST
    ${PROJECT_NAME}_test)

#Include all app own subdirectorys

file(GLOB_RECURSE INTRODUCTION_SRC_TEST ${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp)
#file(GLOB_RECURSE INTRODUCTION_SRC_TEST ${CMAKE_CURRENT_LIST_DIR}/../src/*.)
#file(GLOB_RECURSE INTRODUCTION_SRC_TEST1 ${CMAKE_CURRENT_LIST_DIR}/src/*.)
file(GLOB_RECURSE INTRODUCTION_SRC_TEST1 ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

list(REMOVE_ITEM INTRODUCTION_SRC_TEST "${CMAKE_CURRENT_LIST_DIR}/../src/main.cpp")
list(REMOVE_ITEM INTRODUCTION_SRC_TEST "${CMAKE_CURRENT_LIST_DIR}/../src/dedit/main.cpp")


# 生成测试可执行程序
add_executable(${PROJECT_NAME_TEST}
    ${INTRODUCTION_SRC_TEST}
    ${INTRODUCTION_SRC_TEST1}
    ${APP_QRC}
    )

# 生成测试可执行程序的依赖库
target_link_libraries(${PROJECT_NAME_TEST} 
        gmock gmock_main gtest gtest_main
    ${DtkWidget_LIBRARIES}
    ${DtkCore_LIBRARIES}
    ${DFrameworkdbus_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    ${Qt5PrintSupport_LIBRARIES}
    ${LIBDMR_LIBRARIES}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::DBus
    ${Qt5Widgets_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5Concurrent_LIBRARIES}
    KF5::Codecs
    KF5::SyntaxHighlighting
        )

target_include_directories(${PROJECT_NAME_TEST} PUBLIC ${DtkWidget_INCLUDE_DIRS}
                                                       ${DtkCore_INCLUDE_DIRS}
                                                       ${DtkGui_INCLUDE_DIRS})
                         
add_custom_target(test
    #执行mkdir -p 创建 coverageResult
    COMMAND mkdir -p test/coverageResult
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)                               

add_custom_command(TARGET test
            COMMAND echo " =================== CREAT LCOV REPROT BEGIN ==================== "

    #1.清理原先的gcov信息
    COMMAND lcov --directory ./test/CMakeFiles/${PROJECT_NAME_TEST}.dir --zerocounters
    COMMAND lcov --directory ./test/CMakeFiles/${PROJECT_NAME}.dir --zerocounters
    COMMAND ./test/${PROJECT_NAME_TEST}

    #2.收集gcov信息到.info文件中
    COMMAND lcov --directory . --capture --output-file ./test/coverageResult/${PROJECT_NAME}_Coverage.info

    #3.过滤一些我们不感兴趣的文件的覆盖率信息
    COMMAND echo " =================== do filter begin ==================== "
    COMMAND lcov --remove ./test/coverageResult/${PROJECT_NAME}_Coverage.info
    '*/${PROJECT_NAME_TEST}_autogen/*' '*/${PROJECT_NAME}_autogen/*' '*/usr/include/*' '*/dbuslogin1manager*' '*/test/*' '*/googletest/*'
    -o ./test/coverageResult/${PROJECT_NAME}_Coverage.info
    COMMAND echo " =================== do filter end ==================== "

    #3.将.info信息生成报告到reprot文件夹中
    COMMAND genhtml -o ./test/coverageResult/report ./test/coverageResult/${PROJECT_NAME}_Coverage.info

    COMMAND echo " =================== CREAT LCOV REPROT END ==================== "

    COMMAND echo " Coverage files have been output to ${CMAKE_BINARY_DIR}/coverageResult/report "

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

#'make test'命令依赖与我们的测试程序f
add_dependencies(test ${PROJECT_NAME_TEST})

# 设置添加gocv相关信息的输出
set(CMAKE_CXX_FLAGS "-g -fprofile-arcs -ftest-coverage")

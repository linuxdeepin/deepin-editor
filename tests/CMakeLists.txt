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
set(CMAKE_CXX_FLAGS "-g -fprofile-arcs -ftest-coverage")
set(APP_QRC "../src/deepin-editor.qrc")

# Find the library
find_package(PkgConfig REQUIRED)


set(qt_required_components Gui Widgets DBus Concurrent PrintSupport Svg Xml Test)
if (QT_DESIRED_VERSION MATCHES 6)
    list(APPEND qt_required_components Core5Compat)
endif()
find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS ${qt_required_components})
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget Core)
find_package(KF${KF_VERSION_MAJOR}Codecs REQUIRED)
find_package(KF${KF_VERSION_MAJOR}SyntaxHighlighting REQUIRED)

set(LINK_LIBS
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Test
    Dtk${DTK_VERSION_MAJOR}::Widget
    Dtk${DTK_VERSION_MAJOR}::Core
    KF${KF_VERSION_MAJOR}::Codecs
    KF${KF_VERSION_MAJOR}::SyntaxHighlighting
)

if (QT_DESIRED_VERSION MATCHES 6)
    list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::Core5Compat)
else()
    include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})
endif()

find_package(GTest REQUIRED)
find_package(ICU COMPONENTS i18n uc REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(src)
link_libraries(uchardet)
link_libraries(chardet)

pkg_check_modules(chardet REQUIRED chardet)
include_directories(${chardet_INCLUDE_DIRS})
link_directories(${chardet_LIBRARY_DIRS})

set(PROJECT_NAME_TEST
    ${PROJECT_NAME}-test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline")

#Include all app own subdirectorys


file(GLOB_RECURSE INTRODUCTION_SRC_TEST ${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp ${CMAKE_CURRENT_LIST_DIR}/../src/basepub/*.c)
#file(GLOB_RECURSE INTRODUCTION_SRC_TEST ${CMAKE_CURRENT_LIST_DIR}/../src/*.)
#file(GLOB_RECURSE INTRODUCTION_SRC_TEST1 ${CMAKE_CURRENT_LIST_DIR}/src/*.)
file(GLOB_RECURSE INTRODUCTION_SRC_TEST1 ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp *.theme)
#file(GLOB_RECURSE INTRODUCTION_SRC_TEST1 ${CMAKE_CURRENT_LIST_DIR}/src/*.theme)
file(GLOB_RECURSE INTRODUCTION_SRC_TEST2 ${CMAKE_CURRENT_LIST_DIR}/../src/*.h)

list(REMOVE_ITEM INTRODUCTION_SRC_TEST "${CMAKE_CURRENT_LIST_DIR}/../src/main.cpp")
list(REMOVE_ITEM INTRODUCTION_SRC_TEST "${CMAKE_CURRENT_LIST_DIR}/../src/dedit/main.cpp")
list(REMOVE_ITEM INTRODUCTION_SRC_TEST "${CMAKE_CURRENT_LIST_DIR}/../src/common/encoding.cpp")


# 生成测试可执行程序
add_executable(${PROJECT_NAME_TEST}
    ${INTRODUCTION_SRC_TEST2}
    ${INTRODUCTION_SRC_TEST}
    ${INTRODUCTION_SRC_TEST1}
    ${APP_QRC}
    )

# 生成测试可执行程序的依赖库
target_link_libraries(${PROJECT_NAME_TEST}
        gmock gmock_main gtest gtest_main
    ${LINK_LIBS}
    ${GSTREAMER_LIBRARIES}
    ${LIBDMR_LIBRARIES}
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    ${chardet_LIBRARY_DIRS}
    ICU::i18n
    ICU::uc
    -lpthread
    -lm
    dl
    uchardet
    chardet
)

#以下注释
add_custom_target(test
#    #执行mkdir -p 创建 coverageResult
#    COMMAND mkdir -p test/coverageResult
#    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
#
add_custom_command(TARGET test
    COMMAND echo " =================== CREAT LCOV REPROT BEGIN ==================== "

    COMMAND ${CMAKE_BINARY_DIR}/tests/${PROJECT_NAME_TEST}

    COMMAND echo " =================== TEST END ==================== "
)

##'make test'命令依赖与我们的测试程序f
add_dependencies(test ${PROJECT_NAME_TEST})
#
## 设置添加gocv相关信息的输出
#set(CMAKE_CXX_FLAGS "-g -fprofile-arcs -ftest-coverage")

#ASAN安全性检测标志
set(CMAKE_SAFETYTEST "${CMAKE_SAFETYTEST_ARG}")
if(CMAKE_SAFETYTEST STREQUAL "")
    set(CMAKE_SAFETYTEST "CMAKE_SAFETYTEST_ARG_OFF")
endif()

#代码覆盖率开关
if(CMAKE_COVERAGE_ARG STREQUAL "CMAKE_COVERAGE_ARG_ON")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fprofile-arcs -ftest-coverage")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${CMAKE_SAFETYTEST}")
if(CMAKE_SAFETYTEST STREQUAL "CMAKE_SAFETYTEST_ARG_ON")
  #安全测试选项
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=undefined,address -O2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=undefined,address -O2")
endif()

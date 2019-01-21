cmake_minimum_required(VERSION 3.7)

if (NOT DEFINED VERSION)
    set(VERSION 1.2.2)
endif ()

project(deepin-editor)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "sw_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee")
endif ()

if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

    # generate qm files
    execute_process(COMMAND bash "translate_generation.sh"
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
else ()
    set(CMAKE_CXX_COMPILER "clang++")
endif ()

configure_file(src/environments.h.in environments.h @ONLY)

# Sources files
file(GLOB SRCS "src/*.h" "src/*.cpp"
               "src/thememodule/*.h" "src/thememodule/*.cpp"
               "src/widgets/*.h" "src/widgets/*.cpp")

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

include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

# Tell CMake to create the executable
add_executable(deepin-editor ${SRCS} src/deepin-editor.qrc)
target_include_directories(deepin-editor PUBLIC ${DtkWidget_INCLUDE_DIRS} ${OBJECT_BINARY_DIR})
target_link_libraries(deepin-editor
    ${DtkWidget_LIBRARIES}
    ${DtkCore_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5PrintSupport_LIBRARIES}
    ${Qt5Concurrent_LIBRARIES}

    KF5::Codecs
    KF5::SyntaxHighlighting
)

set(CMAKE_INSTALL_PREFIX /usr)

add_subdirectory("dedit")

# Install qm files
file(GLOB QM_FILES "translations/*.qm")
install(FILES ${QM_FILES}
        DESTINATION share/deepin-editor/translations)

install(TARGETS deepin-editor DESTINATION bin)
install(FILES deepin-editor.desktop DESTINATION share/applications)

install(DIRECTORY src/themes
        DESTINATION share/deepin-editor)

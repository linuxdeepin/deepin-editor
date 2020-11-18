

if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

    # generate qm files
    execute_process(COMMAND bash "${CMAKE_SOURCE_DIR}/translate_generation.sh"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif ()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/environments.h.in ${CMAKE_CURRENT_SOURCE_DIR}/environments.h @ONLY)

# Sources files
file(GLOB SRCS "*.h" "*.cpp"
               "thememodule/*.h" "thememodule/*.cpp"
	       "widgets/*.h" "widgets/*.cpp" "encodes/detectcode.h" "encodes/detectcode.cpp")

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

include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})
include_directories(Qt5PrintSupport)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/lib/include)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../third/lib/lib/libcharset.a)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../third/lib/lib/libiconv.a)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../third/lib/lib/libenca.a)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../third/lib/lib/libuchardet.a)

# Tell CMake to create the executable
add_executable(deepin-editor ${SRCS} deepin-editor.qrc)
target_include_directories(deepin-editor PUBLIC ${DtkWidget_INCLUDE_DIRS} ${OBJECT_BINARY_DIR})
target_link_libraries(deepin-editor
    ${DtkWidget_LIBRARIES}
    ${DtkCore_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5PrintSupport_LIBRARIES}
    ${Qt5Concurrent_LIBRARIES}
    ${DFrameworkdbus_LIBRARIES}
    KF5::Codecs
    KF5::SyntaxHighlighting
)

set(CMAKE_INSTALL_PREFIX /usr)

add_subdirectory("dedit")

# Install qm files
file(GLOB QM_FILES "${CMAKE_SOURCE_DIR}/translations/*.qm")
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-editor/translations/)
install(FILES ${APP_ICONPATH} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)
install(FILES images/deepin-editor.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)

install(TARGETS deepin-editor DESTINATION bin/)
install(FILES ${CMAKE_SOURCE_DIR}/deepin-editor.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications/)
install(DIRECTORY themes DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-editor/)

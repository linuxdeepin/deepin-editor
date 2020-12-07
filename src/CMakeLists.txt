if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    # generate qm files
    execute_process(COMMAND bash "${CMAKE_SOURCE_DIR}/translate_generation.sh" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif ()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/environments.h.in ${CMAKE_CURRENT_SOURCE_DIR}/environments.h @ONLY)

# Sources files
file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h"  "encodes/*.h" "common/*.h" "controls/*.h" "editor/*.h"  "thememodule/*.h" "widgets/*.h")
file(GLOB SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "encodes/*.cpp" "common/*.cpp" "controls/*.cpp" "editor/*.cpp"  "thememodule/*.cpp" "widgets/*.cpp")


include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/encodes)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/controls)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/editor)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thememodule)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/widgets)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/lib/include)


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
find_package(Qt5Xml REQUIRED)
find_package(Qt5Svg REQUIRED)
include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

# Tell CMake to create the executable
add_executable(deepin-editor ${SRCS} deepin-editor.qrc)
target_link_libraries(deepin-editor
    ${DtkWidget_LIBRARIES}
    ${DtkCore_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5PrintSupport_LIBRARIES}
    ${Qt5Concurrent_LIBRARIES}
    ${DFrameworkdbus_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    ${Qt5Svg_LIBRARIES}
    KF5::Codecs
    KF5::SyntaxHighlighting
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/lib/lib/libcharset.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/lib/lib/libiconv.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/lib/lib/libenca.a
    -lm
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/lib/lib/libuchardet.a
)

set(CMAKE_INSTALL_PREFIX /usr)

# Install qm files
file(GLOB QM_FILES "${CMAKE_SOURCE_DIR}/translations/*.qm")
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-editor/translations/)
install(FILES ${APP_ICONPATH} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)
install(FILES ${CMAKE_SOURCE_DIR}/src/images/deepin-editor.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)

install(TARGETS deepin-editor DESTINATION bin/)
install(FILES ${CMAKE_SOURCE_DIR}/deepin-editor.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications/)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/themes DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-editor/)

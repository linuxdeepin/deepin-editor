if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif ()

# Sources files
file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h"  "encodes/*.h" "common/*.h" "controls/*.h" "editor/*.h"  "thememodule/*.h" "widgets/*.h" "basepub/*.h")
file(GLOB SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "encodes/*.cpp" "common/*.cpp" "controls/*.cpp" "editor/*.cpp"  "thememodule/*.cpp" "widgets/*.cpp" "basepub/*.c")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/encodes)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/controls)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/editor)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thememodule)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/widgets)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/basepub)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/lib/include)

# Find the library
find_package(PkgConfig REQUIRED)

set(qt_required_components Gui Widgets DBus Concurrent PrintSupport Svg Xml)
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

find_package(DFrameworkdbus REQUIRED)
find_package(ICU COMPONENTS i18n uc REQUIRED)

pkg_check_modules(chardet REQUIRED chardet)
include_directories(${chardet_INCLUDE_DIRS})
link_directories(${chardet_LIBRARY_DIRS})

# Tell CMake to create the executable
add_executable(deepin-editor ${SRCS} deepin-editor.qrc)
target_link_libraries(deepin-editor
    ${LINK_LIBS}
    ${DFrameworkdbus_LIBRARIES}
    ${chardet_LIBRARY_DIRS}
    ICU::i18n
    ICU::uc
    -lm
    uchardet
    dl
    chardet
)


# translation files
TRANSLATION_GENERATE(QM_FILES ${CMAKE_SOURCE_DIR}/translations)
add_custom_target(${PROJECT_NAME}_qm_files DEPENDS ${QM_FILES})
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_qm_files)

# Install translations
install(FILES ${QM_FILES} DESTINATION share/${PROJECT_NAME}/translations)

install(FILES ${APP_ICONPATH} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)
install(FILES ${CMAKE_SOURCE_DIR}/src/images/deepin-editor.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)

#代码覆盖率开关
if(CMAKE_COVERAGE_ARG STREQUAL "CMAKE_COVERAGE_ARG_ON")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fprofile-arcs -ftest-coverage")
endif()

# Install help manual
#if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "loongarch64")
#    install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/assets/loongarch64/deepin-editor DESTINATION /usr/share/deepin-manual/manual-assets/application/)
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "sw_64")
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/assets/loongarch64/deepin-editor DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-manual/manual-assets/application/)
else()
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/assets/deepin-editor DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-manual/manual-assets/application/)
endif()

install(TARGETS deepin-editor DESTINATION bin/)
install(FILES ${CMAKE_SOURCE_DIR}/deepin-editor.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications/)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/themes DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-editor/)

# 安装 DConfig 配置文件，1040及以下的默认DTK环境不支持DConfig
set(DCONFIG_APPID org.deepin.editor)
file(GLOB DCONFIG_FILES "${CMAKE_SOURCE_DIR}/misc/configs/*.json")
if(DEFINED DSG_DATA_DIR)
    if (DTK_VERSION_MAJOR EQUAL "6")
        dtk_add_config_meta_files(APPID ${DCONFIG_APPID} FILES ${DCONFIG_FILES})
    else()
        dconfig_meta_files(APPID ${DCONFIG_APPID} FILES ${DCONFIG_FILES})
    endif()
    message("-- DConfig is supported by DTK")
else()
    install(FILES ${DCONFIG_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dsg/configs/${DCONFIG_APPID})
    message("-- DConfig is not supported by DTK")
endif()

# 添加KF5格式高亮配置文件，/usr/share/org.kde.syntax-highlighting/syntax/ 为所有用户可读取高亮配置的路径
install(DIRECTORY ${CMAKE_SOURCE_DIR}/misc/org.kde.syntax-highlighing/syntax
    DESTINATION ${KF5_HIGHLIGHT_INSTALL_PATH})

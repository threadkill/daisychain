cmake_minimum_required (VERSION 3.12.0)
project (chain)

add_compile_definitions (ELPP_NO_LOG_TO_FILE=1)

if (APPLE)
# suppresses ranlib warnings for no symbols found.
SET(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()


# Qt6
find_package (Qt6 COMPONENTS Widgets REQUIRED)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)


# NodeEditor
set (CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set (BUILD_SHARED_LIBS OFF)
add_subdirectory (
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/nodeeditor
    ${CMAKE_CURRENT_BINARY_DIR}/nodeeditor
    EXCLUDE_FROM_ALL
)


# Source
include_directories (
    src
)

set (chain_SOURCES
    resources/chainresources.qrc
    src/chain.cpp
    src/chainnotes.cpp
    src/chainbrowser.cpp
    src/chainmodel.cpp
    src/chainscene.cpp
    src/chainview.cpp
    src/chainwindow.cpp
    src/commandmodel.cpp
    src/concatmodel.cpp
    src/distromodel.cpp
    src/filelistmodel.cpp
    src/watchmodel.cpp
    src/environwidget.cpp
    src/filtermodel.cpp
    src/logwidget.cpp
    src/lognotifier.cpp
    src/loghighlighter.cpp
    src/graphmodel.cpp
    src/graphmodel.h
    src/logfilter.cpp
    src/logfilter.h
)


if (APPLE)
    list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    set (MACOSX_BUNDLE_BUNDLE_NAME "DaisyChain")
    set (MACOSX_BUNDLE_ICON_FILE daisychain.icns)
    set (chain_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/daisychain.icns)
    set_source_files_properties (${chain_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable (chain MACOSX_BUNDLE ${chain_ICON} ${chain_SOURCES})
    install(
        CODE "
        include (BundleUtilities)
        copy_and_fixup_bundle (${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app ${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_BUNDLE_NAME}.app \"\" \"\")"
        COMPONENT Runtime
    )
else()
    add_executable (chain ${chain_SOURCES})
    install (TARGETS chain DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()


target_link_libraries (chain PRIVATE pthread daisychain_static QtNodes Qt6::Widgets)
set_target_properties (chain PROPERTIES CXX_VISIBILITY_PRESET "hidden")

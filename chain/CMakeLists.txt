cmake_minimum_required (VERSION 3.21.1)
project (chain)

# Define compile options
add_compile_definitions (ELPP_NO_LOG_TO_FILE=1)

if (APPLE)
    # Suppresses ranlib warnings for no symbols found
    set (CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set (CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set (CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    set (CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

# Qt6
find_package (Qt6 COMPONENTS Widgets REQUIRED)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

# NodeEditor
add_custom_target (apply_nodeeditor_patch
    COMMAND ${CMAKE_COMMAND} -E echo "Applying patch to NodeEditor..."
    COMMAND git checkout nodeeditor/src/AbstractNodeGeometry.cpp
    COMMAND git apply ${CMAKE_SOURCE_DIR}/patches/nodeeditor.patch
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdparty
    COMMENT "Patching NodeEditor before building"
)
set (CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set (BUILD_SHARED_LIBS OFF)
add_subdirectory (
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/nodeeditor
    ${CMAKE_CURRENT_BINARY_DIR}/nodeeditor
    EXCLUDE_FROM_ALL
)
add_dependencies (QtNodes apply_nodeeditor_patch)

# Include source directories
include_directories (
    src
)

# Define source files
set (chain_SOURCES
    resources/chainresources.qrc
    src/chain.cpp
    src/chain.h
    src/chainbrowser.cpp
    src/chainbrowser.h
    src/chainfonts.h
    src/chainmodel.cpp
    src/chainmodel.h
    src/chainnotes.cpp
    src/chainnotes.h
    src/chainreaddata.h
    src/chainscene.cpp
    src/chainscene.h
    src/chainstyles.h
    src/chainview.cpp
    src/chainview.h
    src/chainwindow.cpp
    src/chainwindow.h
    src/chainwritedata.h
    src/commandmodel.cpp
    src/commandmodel.h
    src/concatmodel.cpp
    src/concatmodel.h
    src/distromodel.cpp
    src/distromodel.h
    src/environwidget.cpp
    src/environwidget.h
    src/filelistmodel.cpp
    src/filelistmodel.h
    src/filtermodel.cpp
    src/filtermodel.h
    src/graphmodel.cpp
    src/graphmodel.h
    src/logfilter.cpp
    src/logfilter.h
    src/loghighlighter.cpp
    src/loghighlighter.h
    src/logwidget.cpp
    src/logwidget.h
    src/watchmodel.cpp
    src/watchmodel.h
)

# macOS specific configurations
if (APPLE)
    find_program (Qt6_TOOL_PATH
        NAMES macdeployqt6
        HINTS "${Qt6_DIR}/bin"
    )
    if (Qt6_TOOL_PATH)
        message (STATUS "Found Qt6 binary: ${Qt6_TOOL_PATH}")
    else()
        message (FATAL_ERROR "Qt6 binary ${Qt6_TOOL_PATH} not found!")
    endif()

    list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    set (MACOSX_BUNDLE_BUNDLE_NAME "DaisyChain")
    set (MACOSX_BUNDLE_ICON_FILE daisychain.icns)
    set (chain_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/daisychain.icns)
    set_source_files_properties (${chain_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable (chain MACOSX_BUNDLE ${chain_ICON} ${chain_SOURCES})
    install (
        CODE "
        include (BundleUtilities)
        copy_and_fixup_bundle (${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app \
            ${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_BUNDLE_NAME}.app \"\" \"\")
        message (STATUS \"Running ${Qt6_TOOL_PATH} command...\")
        execute_process (
            COMMAND ${Qt6_TOOL_PATH} ${MACOSX_BUNDLE_BUNDLE_NAME}.app \
                    -always-overwrite
            WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX})"
        COMPONENT Runtime
    )
    install (
        CODE "
        message (STATUS \"Running ${Qt6_TOOL_PATH} command...\")
        execute_process (
            COMMAND ${Qt6_TOOL_PATH} ${MACOSX_BUNDLE_BUNDLE_NAME}.app \
                -always-overwrite -dmg
            WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX})"
    )

# Windows specific configurations
elseif (WIN32)
    find_program (Qt6_TOOL_PATH
        NAMES windeployqt
        HINTS "${Qt6_DIR}/bin"
    )
    if (Qt6_TOOL_PATH)
        message (STATUS "Found Qt6 binary: ${Qt6_TOOL_PATH}")
    else()
        message (FATAL_ERROR "Qt6 binary ${Qt6_TOOL_PATH} not found!")
    endif()

    list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    set (CHAIN_ICON "${CMAKE_SOURCE_DIR}/chain/resources/win_icon.rc")

    # Define executable
    add_executable (chain WIN32 ${chain_SOURCES})
    target_sources (chain PRIVATE ${CHAIN_ICON})
    set_target_properties (chain PROPERTIES OUTPUT_NAME "DaisyChain")
    qt_disable_unicode_defines (chain)
    set (WINDEPLOYQT_TMP "${CMAKE_BINARY_DIR}/windeployqt_tmp")

    # Install the executable and run windeployqt to bundle Qt6
    install (TARGETS chain DESTINATION .)
    install (
        CODE "
        execute_process(
            COMMAND ${Qt6_TOOL_PATH} --release --compiler-runtime --dir ${WINDEPLOYQT_TMP} \$<TARGET_FILE:chain>
            WORKING_DIRECTORY \$<TARGET_FILE_DIR:chain>)"
    )
    install (DIRECTORY "${WINDEPLOYQT_TMP}/" DESTINATION .)
    install (FILES "${CMAKE_SOURCE_DIR}/COPYING" DESTINATION . RENAME license.txt)

    # CPack configuration for packaging the *.exe installer
    include (InstallRequiredSystemLibraries)
    set (CPACK_PACKAGE_NAME "DaisyChain")
    set (CPACK_PACKAGE_VERSION ${DAISYCHAIN_VERSION})
    set (CPACK_PACKAGE_CONTACT "admin@threadkill.io")
    set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Qt6 Application.")
    set (CPACK_PACKAGE_VENDOR "threadkill, Inc.")
    set (CPACK_PACKAGE_INSTALL_DIRECTORY "DaisyChain")
    set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
    set (CPACK_GENERATOR "NSIS")
    set (CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/chain/resources/daisy-icon.ico")

    set (CPACK_NSIS_CREATE_ICONS_EXTRA "
        CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\DaisyChain.lnk' '$INSTDIR\\\\DaisyChain.exe'
        CreateShortCut '$DESKTOP\\\\DaisyChain.lnk' '$INSTDIR\\\\DaisyChain.exe'"
    )
    set (CPACK_NSIS_DELETE_ICONS_EXTRA "
        Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\DaisyChain.lnk'
        Delete '$DESKTOP\\\\DaisyChain.lnk'
        RMDir '$SMPROGRAMS\\\\$STARTMENU_FOLDER'"
    )
    include (CPack)

    # Add a custom target to run CPack after installation
    add_custom_target (run_cpack
        COMMAND ${CMAKE_COMMAND} -E echo "Running CPack to generate package..."
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR} ${CMAKE_CPACK_COMMAND} -C ${CMAKE_BUILD_TYPE}
        DEPENDS install
    )

    if (MSVC)
        target_compile_options (chain PRIVATE /W3 /wd4100)
        target_compile_options (QtNodes INTERFACE /W3)
        target_link_options (chain PRIVATE /IGNORE:4217 /IGNORE:4286)
    endif()

# Linux et al
else()
    add_executable (chain ${chain_SOURCES})
    install (TARGETS chain DESTINATION .)
endif()

add_custom_command (TARGET chain
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Running post-build command for QtNodes..."
    COMMAND git checkout nodeeditor/src/AbstractNodeGeometry.cpp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdparty
    COMMENT "Post-build git command for reverting patched QtNodes"
)

# Link necessary libraries
target_link_libraries (chain PRIVATE daisychain_static Qt6::Widgets QtNodes)
set_target_properties (chain PROPERTIES CXX_VISIBILITY_PRESET "hidden")

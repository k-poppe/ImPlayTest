cmake_minimum_required(VERSION 3.13)

find_package(Git)
if(GIT_EXECUTABLE)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --match "[0-9]*.[0-9]*.[0-9]*" --abbrev=8
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE status
        OUTPUT_VARIABLE GIT_VERSION
    )
    if (${status})
        set(GIT_VERSION "0.0.0")
    else()
        string(STRIP ${GIT_VERSION} GIT_VERSION)
        string(REGEX REPLACE "-[0-9]+-g" "-" GIT_VERSION ${GIT_VERSION})
    endif()
else()
    set(GIT_VERSION "0.0.0")
endif()
string(REGEX MATCH "^[0-9]+.[0-9]+.[0-9]+" SEMVER "${GIT_VERSION}")
message("-- Git Tag: ${GIT_VERSION}, SemVer: ${SEMVER}")

project(ImPlay VERSION "${SEMVER}")

set(CMAKE_CXX_STANDARD 20)

option(USE_MPV_WIN_BUILD "Use Prebuilt static mpv dll for Windows" ON)
option(USE_PATCHED_GLFW "Use patched GLFW to support additional features" ON)

include(ExternalProject)
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14.0")
    cmake_policy(SET CMP0087 NEW)
endif()
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

find_package(PkgConfig REQUIRED)
if(WIN32)
    ExternalProject_Add(mpv_bin
        URL https://github.com/mpv-player/mpv/archive/v0.31.0.tar.gz
        CMAKE_ARGS -DENABLE_STATIC=1 -DENABLE_SHARED=0 -DENABLE_DOCS=0 -DENABLE_MANPAGE=0 -DENABLE_HTML_BUILD=0 -DENABLE_JACK=0 -DENABLE_PULSEAUDIO=0 -DENABLE_ALSA=0 -DENABLE_OSS=0 -DENABLE_CACA=0 -DENABLE_RPI=0 -DENABLE_VAAPI=0 -DENABLE_VDPAU=0 -DENABLE_CUDA=0 -DENABLE_OPENCL=0 -DENABLE_EGL=0 -DENABLE_GL_X11=0 -DENABLE_GL_COCOA=0 -DENABLE_GL_WIN32=0 -DENABLE_GL_WAYLAND=0 -DENABLE_GL_XCB=0 -DENABLE_GL_EGL=0 -DENABLE_GL_VULKAN=0 -DENABLE_GL_D3D11=0 -DENABLE_VAPOURSYNTH=0 -DENABLE_LAVF=0 -DENABLE_AVDEVICE=0 -DENABLE_AVFILTER=0 -DENABLE_POSTPROC=0 -DENABLE_ENCODING=0 -DENABLE_LIBASS=0 -DENABLE_ZIMG=0 -DENABLE_FREI0R=0 -DENABLE_LIBCDIO=0 -DENABLE_LIBCDIO_PARANOIA=0 -DENABLE_DVDNAV=0 -DENABLE_HLS=1 -DENABLE_MP4=1
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
        BUILD_COMMAND make
        INSTALL_COMMAND make install
    )
    ExternalProject_Get_property(mpv_bin BINARY_DIR)
    set(MPV_BIN_DIR ${BINARY_DIR})
    set(MPV_INCLUDE_DIRS ${MPV_BIN_DIR}/include)
    set(MPV_LIBRARY_DIRS ${MPV_BIN_DIR}/lib)
    set(MPV_LIBRARIES mpv)
else()
    pkg_search_module(MPV REQUIRED mpv)
endif()

if(USE_PATCHED_GLFW)
    add_subdirectory(third_party/glfw)
    set(GLFW_LIBRARIES glfw)
else()
    pkg_search_module(GLFW REQUIRED glfw3)
endif()

set(LIBROMFS_PROJECT_NAME ${PROJECT_NAME})
set(LIBROMFS_RESOURCE_LOCATION "${CMAKE_SOURCE_DIR}/resources/romfs")

add_subdirectory(third_party/fmt)
add_subdirectory(third_party/json)
add_subdirectory(third_party/inipp)
add_subdirectory(third_party/imgui)
add_subdirectory(third_party/nativefiledialog)
add_subdirectory(third_party/libromfs)

set(SOURCE_FILES
    source/views/view.cpp
    source/views/command_palette.cpp
    source/views/context_menu.cpp
    source/views/debug.cpp
    source/views/about.cpp
    source/views/settings.cpp
    source/helpers.cpp
    source/command.cpp
    source/config.cpp
    source/dispatch.cpp
    source/mpv.cpp
    source/player.cpp
    source/window.cpp
    source/main.cpp
)
set(INCLUDE_DIRS include ${MPV_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS})
set(LINK_LIBS fmt json inipp nfd imgui ${MPV_LIBRARIES} ${GLFW_LIBRARIES} ${LIBROMFS_LIBRARY})

if(WIN32)
    configure_file(${PROJECT_SOURCE_DIR}/resources/win32/app.rc.in ${PROJECT_BINARY_DIR}/app.rc @ONLY)
    list(APPEND SOURCE_FILES ${PROJECT_BINARY_DIR}/app.rc)
elseif(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_ICON_FILE "AppIcon")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.tsl0922.ImPlay")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2022 tsl0922. All rights reserved." )
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${MPV_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${GIT_VERSION}")
if(USE_PATCHED_GLFW)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_PATCHED)
endif()

if(WIN32)
    
    ExternalProject_Add(yt_dlp
        URL https://github.com/yt-dlp/yt-dlp/releases/download/2023.01.06/yt-dlp.exe
        DOWNLOAD_NO_EXTRACT ON
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )
    add_dependencies(${PROJECT_NAME} mpv_bin yt_dlp)
    ExternalProject_Get_property(mpv_bin BINARY_DIR)
    set(MPV_BIN_DIR ${BINARY_DIR})
    ExternalProject_Get_property(yt_dlp DOWNLOAD_DIR)
    set(YT_DLP_BIN ${DOWNLOAD_DIR}/yt-dlp.exe)

    target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
    install(FILES ${MPV_BIN_DIR}/mpv.com DESTINATION . RENAME ImPlay.com)
    install(FILES ${YT_DLP_BIN} DESTINATION .)
    if(USE_MPV_WIN_BUILD)
        add_dependencies(${PROJECT_NAME} mpv_bin)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MPV_DEV_DIR}/libmpv-2.dll
                $<TARGET_FILE_DIR:ImPlay>
        )
    endif()
    
    install(CODE [[
    file(GET_RUNTIME_DEPENDENCIES
        EXECUTABLES $<TARGET_FILE:ImPlay>
        RESOLVED_DEPENDENCIES_VAR _r_deps
        UNRESOLVED_DEPENDENCIES_VAR _u_deps
        DIRECTORIES $ENV{PATH}
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
    )

    if(_u_deps)
        message(WARNING "Unresolved dependencies detected: \"${_u_deps}\"!")
    endif()

    foreach(_file ${_r_deps})
        file(INSTALL
            DESTINATION ${CMAKE_INSTALL_PREFIX}
            TYPE SHARED_LIBRARY
            FOLLOW_SYMLINK_CHAIN
            FILES "${_file}"
        )
    endforeach()
    ]])

    set(CPACK_GENERATOR ZIP WIX)
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-win64")
    set(CPACK_WIX_PATCH_FILE "${PROJECT_SOURCE_DIR}/resources/win32/wix/patch.xml")
    set(CPACK_WIX_PRODUCT_ICON "${PROJECT_SOURCE_DIR}/resources/win32/app.ico")
    set(CPACK_WIX_UPGRADE_GUID "D7438EFE-D62A-4E94-A024-6E71AE1A7A63")
    set_property(INSTALL "$<TARGET_FILE_NAME:ImPlay>" PROPERTY CPACK_START_MENU_SHORTCUTS "ImPlay")
elseif (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/resources/macos/Info.plist.in
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath
            "@executable_path/../Frameworks/"
            $<TARGET_FILE:ImPlay>
    )
    
    target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/resources/macos/AppIcon.icns)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/resources/macos/AppIcon.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources
    )

    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)
    
    set(APPS "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app")
    set(DIRS ${CMAKE_BINARY_DIR})
    install(CODE "include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\" IGNORE_ITEM Python)"
    )
    
    set(CPACK_GENERATOR DragNDrop)
    set(CPACK_BUNDLE_NAME ${PROJECT_NAME})
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-macOS")
    set(CPACK_BUNDLE_ICON ${PROJECT_SOURCE_DIR}/resources/macos/app.icns)
    set(CPACK_BUNDLE_PLIST ${CMAKE_BINARY_DIR}/ImHex.app/Contents/Info.plist)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_GENERATOR TGZ DEB)
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-linux64")
    set(CPACK_DEBIAN_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "tsl0922")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS TRUE)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
else()
    set(CPACK_GENERATOR TGZ)
endif()

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "tsl0922")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A Cross-Platform Desktop Media Player")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)

include(CPack)

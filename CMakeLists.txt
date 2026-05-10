# mtype-imgui-sdl — standalone runtime plugin for mType.
#
# Builds mtype_sdl_imgui.{dll,so,dylib}, loaded by mType scripts via
# __plugin_load. Depends only on the C ABI declared in include/PluginHostApi.h
# (vendored from mType — keep in sync if MTYPE_PLUGIN_ABI_VERSION bumps),
# vendor/SDL3, and vendor/imgui (docking branch).

cmake_minimum_required(VERSION 3.16)
project(mtype_sdl_imgui CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---- vendor: SDL3 ----------------------------------------------------------
# Build SDL3 as part of the plugin build. EXCLUDE_FROM_ALL keeps SDL3's test
# binaries / installers out of the default target; we only pull in SDL3::SDL3.
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS        OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL      OFF CACHE BOOL "" FORCE)
set(SDL_SHARED       ON  CACHE BOOL "" FORCE)
set(SDL_STATIC       OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/SDL3 EXCLUDE_FROM_ALL)

# ---- vendor: ImGui ---------------------------------------------------------
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

# ---- the plugin shared lib -------------------------------------------------
set(PLUGIN_SOURCES
    src/PluginEntry.cpp
    src/SdlBindings.cpp
    src/ImGuiBindings.cpp
    src/HandleRegistry.hpp
    src/PluginGlobals.hpp
)

add_library(mtype_sdl_imgui SHARED
    ${PLUGIN_SOURCES}
    ${IMGUI_SOURCES}
)

# Plugin authors include "PluginHostApi.h" by name — surface that header,
# the plugin sources themselves, and the ImGui headers.
target_include_directories(mtype_sdl_imgui PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(mtype_sdl_imgui PRIVATE SDL3::SDL3)

# Output naming: same on every platform so .mt scripts can use one literal.
set_target_properties(mtype_sdl_imgui PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mtype_sdl_imgui"
)

# Linux: visibility hidden by default — only the explicit MTYPE_PLUGIN_EXPORT
# symbols leak. (Windows already opt-in via __declspec(dllexport).)
if(UNIX AND NOT APPLE)
    set_target_properties(mtype_sdl_imgui PROPERTIES
        SUFFIX ".so"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()

# Convenience: copy SDL3.dll next to the plugin's output on Windows so a
# user copying the plugin DLL alone gets SDL3 with it.
if(WIN32)
    add_custom_command(TARGET mtype_sdl_imgui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3-shared>
            $<TARGET_FILE_DIR:mtype_sdl_imgui>
        COMMENT "Copying SDL3.dll alongside mtype_sdl_imgui.dll"
    )
endif()

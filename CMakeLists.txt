cmake_minimum_required(VERSION 3.16)

project(RidgeDash VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(RIDGEDASH_BIN_NAME "RidgeDash" CACHE STRING "Output executable name")
if(RIDGEDASH_BIN_NAME STREQUAL "M5CardputerZero-RidgeDash")
    set(RIDGEDASH_BIN_NAME "RidgeDash" CACHE STRING "Output executable name" FORCE)
endif()
set(RIDGEDASH_RAYLIB_PLATFORM "Desktop" CACHE STRING "Render platform: Desktop, DRM, or FBDEV")

set(RIDGEDASH_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(RIDGEDASH_SRC_DIR "${RIDGEDASH_ROOT}/src")
set(RIDGEDASH_DEPS_DIR "${RIDGEDASH_ROOT}/dependencies")
set(RIDGEDASH_RAYLIB_DIR "${RIDGEDASH_DEPS_DIR}/raylib")
set(RIDGEDASH_BOX2D_DIR "${RIDGEDASH_DEPS_DIR}/box2d")
set(RIDGEDASH_SMOOTH_UITK_DIR "${RIDGEDASH_DEPS_DIR}/smooth_ui_toolkit")
set(RIDGEDASH_SYSTEM_LIBS ${CMAKE_DL_LIBS})

if(UNIX AND NOT APPLE)
    list(APPEND RIDGEDASH_SYSTEM_LIBS m)
endif()

foreach(required_dir IN LISTS RIDGEDASH_RAYLIB_DIR RIDGEDASH_BOX2D_DIR RIDGEDASH_SMOOTH_UITK_DIR)
    if(NOT EXISTS "${required_dir}/CMakeLists.txt")
        message(FATAL_ERROR "Missing dependency: ${required_dir}. Run 'python3 fetch_repos.py' first.")
    endif()
endforeach()

set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_VALIDATE OFF CACHE BOOL "" FORCE)
set(SMOOTH_UI_TOOLKIT_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SMOOTH_UI_TOOLKIT_BUILD_TEST OFF CACHE BOOL "" FORCE)

if(NOT RIDGEDASH_RAYLIB_PLATFORM STREQUAL "FBDEV")
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    # Audio (raudio/miniaudio) is only enabled on Desktop for now. DRM/embedded
    # builds keep it off to avoid pulling in ALSA/PulseAudio deps.
    if(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Desktop")
        set(SUPPORT_MODULE_RAUDIO ON CACHE BOOL "" FORCE)
    else()
        set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)
    endif()
    set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
    set(PLATFORM "${RIDGEDASH_RAYLIB_PLATFORM}" CACHE STRING "" FORCE)
    add_subdirectory("${RIDGEDASH_RAYLIB_DIR}" EXCLUDE_FROM_ALL)
endif()

add_subdirectory("${RIDGEDASH_BOX2D_DIR}" EXCLUDE_FROM_ALL)
add_subdirectory("${RIDGEDASH_SMOOTH_UITK_DIR}" EXCLUDE_FROM_ALL)

file(GLOB_RECURSE RIDGEDASH_CPP CONFIGURE_DEPENDS
    "${RIDGEDASH_SRC_DIR}/*.cpp"
)

add_executable(${RIDGEDASH_BIN_NAME}
    ${RIDGEDASH_CPP}
)

# macOS native fullscreen hook (Cocoa). Only compiled for the desktop Apple build;
# other platforms use native_window_stub.cpp (globbed above). Compiled as
# Objective-C++ via `-x objective-c++` on the C++ toolchain, so we avoid enabling a
# global OBJCXX language (which would break GLFW's plain Objective-C .m files).
if(APPLE AND RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Desktop")
    set_source_files_properties(
        "${RIDGEDASH_SRC_DIR}/platform/native_window_mac.mm"
        PROPERTIES LANGUAGE CXX COMPILE_OPTIONS "-x;objective-c++"
    )
    target_sources(${RIDGEDASH_BIN_NAME} PRIVATE
        "${RIDGEDASH_SRC_DIR}/platform/native_window_mac.mm"
    )
    target_link_libraries(${RIDGEDASH_BIN_NAME} PRIVATE
        "-framework AppKit"
    )
endif()

target_include_directories(${RIDGEDASH_BIN_NAME} PRIVATE
    "${RIDGEDASH_SRC_DIR}"
)

if(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "FBDEV")
    target_include_directories(${RIDGEDASH_BIN_NAME} PRIVATE
        "${RIDGEDASH_RAYLIB_DIR}/src/external"
    )
    target_compile_definitions(${RIDGEDASH_BIN_NAME} PRIVATE
        RIDGEDASH_USE_FBDEV=1
    )
elseif(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "DRM")
    target_compile_definitions(${RIDGEDASH_BIN_NAME} PRIVATE
        RIDGEDASH_DRM_RENDER=1
        RIDGEDASH_USE_EVDEV_INPUT=1
    )
elseif(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Desktop")
    target_compile_definitions(${RIDGEDASH_BIN_NAME} PRIVATE
        RIDGEDASH_DESKTOP_RENDER=1
        RIDGEDASH_ENABLE_AUDIO=1
    )
endif()

target_compile_definitions(${RIDGEDASH_BIN_NAME} PRIVATE
    RIDGEDASH_SCREEN_WIDTH=320
    RIDGEDASH_SCREEN_HEIGHT=170
)

target_link_libraries(${RIDGEDASH_BIN_NAME} PRIVATE
    box2d
    smooth_ui_toolkit
    ${RIDGEDASH_SYSTEM_LIBS}
)

if(NOT RIDGEDASH_RAYLIB_PLATFORM STREQUAL "FBDEV")
    target_link_libraries(${RIDGEDASH_BIN_NAME} PRIVATE raylib)
endif()

set_target_properties(${RIDGEDASH_BIN_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime"
)

add_custom_target(copy_${RIDGEDASH_BIN_NAME}_dist ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RIDGEDASH_ROOT}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${RIDGEDASH_BIN_NAME}>"
        "${RIDGEDASH_ROOT}/dist/${RIDGEDASH_BIN_NAME}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${RIDGEDASH_ROOT}/assets"
        "${RIDGEDASH_ROOT}/dist/assets"
    COMMAND ${CMAKE_COMMAND}
        -DDIST_ASSETS=${RIDGEDASH_ROOT}/dist/assets
        -P "${RIDGEDASH_ROOT}/cmake/strip_dist_assets.cmake"
    DEPENDS ${RIDGEDASH_BIN_NAME}
    VERBATIM
)

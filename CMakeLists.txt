cmake_minimum_required(VERSION 3.16)

project(RidgeDash VERSION 1.0.0 LANGUAGES C CXX)

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
    # Audio (raudio/miniaudio) is enabled on Desktop, Web, and DRM (e.g. Pi Zero
    # with ALSA). FBDEV builds keep it off — no audio stack is assumed.
    if(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Desktop" OR RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Web" OR RIDGEDASH_RAYLIB_PLATFORM STREQUAL "DRM")
        set(SUPPORT_MODULE_RAUDIO ON CACHE BOOL "" FORCE)
    else()
        set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)
    endif()
    set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
    set(PLATFORM "${RIDGEDASH_RAYLIB_PLATFORM}" CACHE STRING "" FORCE)
    # Web target: request a WebGL2 (OpenGL ES 3.0) context so that raylib's
    # emscripten_webgl_create_context call uses majorVersion=2.  Without this,
    # raylib defaults to ES2/WebGL1 which rejects resizable ArrayBufferViews
    # from the growing WASM heap (the texImage2D error seen at runtime).
    if(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Web")
        set(GRAPHICS "GRAPHICS_API_OPENGL_ES3" CACHE STRING "" FORCE)
    endif()
    add_subdirectory("${RIDGEDASH_RAYLIB_DIR}" EXCLUDE_FROM_ALL)
endif()

add_subdirectory("${RIDGEDASH_BOX2D_DIR}" EXCLUDE_FROM_ALL)
add_subdirectory("${RIDGEDASH_SMOOTH_UITK_DIR}" EXCLUDE_FROM_ALL)

file(GLOB_RECURSE RIDGEDASH_CPP CONFIGURE_DEPENDS
    "${RIDGEDASH_SRC_DIR}/*.cpp"
)
# main_web.cpp is the Emscripten entry point; always remove it from the glob so
# it is never accidentally compiled into non-Web targets.
list(REMOVE_ITEM RIDGEDASH_CPP "${RIDGEDASH_SRC_DIR}/main_web.cpp")
if(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Web")
    # Swap the standard blocking main for the emscripten_set_main_loop variant.
    list(REMOVE_ITEM RIDGEDASH_CPP "${RIDGEDASH_SRC_DIR}/main.cpp")
    list(APPEND RIDGEDASH_CPP "${RIDGEDASH_SRC_DIR}/main_web.cpp")
endif()

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
        RIDGEDASH_ENABLE_AUDIO=1
    )
elseif(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Desktop")
    target_compile_definitions(${RIDGEDASH_BIN_NAME} PRIVATE
        RIDGEDASH_DESKTOP_RENDER=1
        RIDGEDASH_ENABLE_AUDIO=1
    )
elseif(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Web")
    target_compile_definitions(${RIDGEDASH_BIN_NAME} PRIVATE
        RIDGEDASH_WEB=1
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

# --- Emscripten / Web link configuration ------------------------------------
if(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Web")
    # Output as a self-contained HTML page alongside its .js/.wasm/.data files.
    set_target_properties(${RIDGEDASH_BIN_NAME} PROPERTIES SUFFIX ".html")

    set(RIDGEDASH_ASSETS_DIR "${RIDGEDASH_ROOT}/assets")
    target_link_options(${RIDGEDASH_BIN_NAME} PRIVATE
        -sUSE_GLFW=3
        -sWASM=1
        -sUSE_WEBGL2=1              # require WebGL2 (OpenGL ES 3.0)
        -sGL_ENABLE_GET_PROC_ADDRESS=1
        # Fixed heap size: ALLOW_MEMORY_GROWTH causes Emscripten 3.x to back the
        # WASM heap with a JS ResizableArrayBuffer, which Chrome's WebGL rejects in
        # texImage2D regardless of WebGL version.  A fixed allocation keeps the
        # ArrayBuffer non-resizable and avoids the error entirely.
        -sINITIAL_MEMORY=134217728  # 128 MB fixed heap
        # Embed the assets directory into the virtual FS so FileExists() / file
        # loading work the same as on Desktop (paths starting with assets/).
        "SHELL:--preload-file ${RIDGEDASH_ASSETS_DIR}@/assets"
        "SHELL:--shell-file ${RIDGEDASH_ROOT}/src/web/shell.html"
    )
endif()

# --- Dist copy target -------------------------------------------------------
if(RIDGEDASH_RAYLIB_PLATFORM STREQUAL "Web")
    # Web build: copy the generated HTML/JS/WASM/DATA bundle to dist/.
    # Assets are embedded in the .data file; no separate assets/ copy needed.
    set(RIDGEDASH_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/runtime")
    add_custom_target(copy_${RIDGEDASH_BIN_NAME}_dist ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RIDGEDASH_ROOT}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RIDGEDASH_RUNTIME_DIR}/${RIDGEDASH_BIN_NAME}.html"
            "${RIDGEDASH_ROOT}/dist/index.html"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RIDGEDASH_RUNTIME_DIR}/${RIDGEDASH_BIN_NAME}.js"
            "${RIDGEDASH_ROOT}/dist/${RIDGEDASH_BIN_NAME}.js"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RIDGEDASH_RUNTIME_DIR}/${RIDGEDASH_BIN_NAME}.wasm"
            "${RIDGEDASH_ROOT}/dist/${RIDGEDASH_BIN_NAME}.wasm"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RIDGEDASH_RUNTIME_DIR}/${RIDGEDASH_BIN_NAME}.data"
            "${RIDGEDASH_ROOT}/dist/${RIDGEDASH_BIN_NAME}.data"
        DEPENDS ${RIDGEDASH_BIN_NAME}
        VERBATIM
    )
else()
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
endif()

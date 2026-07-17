# Emscripten/Web platform configuration.

set(RIDGEDASH_PLATFORM_USES_RAYLIB ON)
set(RIDGEDASH_PLATFORM_RAYLIB_NAME "Web")
set(RIDGEDASH_PLATFORM_RAYLIB_GRAPHICS "GRAPHICS_API_OPENGL_ES3")
set(RIDGEDASH_PLATFORM_ENABLE_AUDIO ON)
set(RIDGEDASH_PLATFORM_IS_WEB ON)
set(RIDGEDASH_PLATFORM_COMPILE_DEFINITIONS
    RIDGEDASH_WEB=1
    RIDGEDASH_ENABLE_AUDIO=1
)
set(RIDGEDASH_PLATFORM_LINK_LIBRARIES raylib)
set(RIDGEDASH_PLATFORM_TARGET_SUFFIX ".html")
# Use WebGL2 and a fixed WASM heap. A growing heap is backed by a resizable
# ArrayBuffer, which browsers reject when raylib passes its views to texImage2D.
# Assets are preloaded into the same /assets path used by native builds.
set(RIDGEDASH_PLATFORM_LINK_OPTIONS
    -sUSE_GLFW=3
    -sWASM=1
    -sUSE_WEBGL2=1
    -sGL_ENABLE_GET_PROC_ADDRESS=1
    -sINITIAL_MEMORY=134217728
    "SHELL:--preload-file ${RIDGEDASH_ROOT}/assets@/assets"
    "SHELL:--shell-file ${RIDGEDASH_ROOT}/src/web/shell.html"
)

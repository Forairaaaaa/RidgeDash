# Desktop platform configuration (Linux, macOS, and Windows).

set(RIDGEDASH_PLATFORM_USES_RAYLIB ON)
set(RIDGEDASH_PLATFORM_RAYLIB_NAME "Desktop")
set(RIDGEDASH_PLATFORM_ENABLE_AUDIO ON)
set(RIDGEDASH_PLATFORM_COMPILE_DEFINITIONS
    RIDGEDASH_DESKTOP_RENDER=1
    RIDGEDASH_ENABLE_AUDIO=1
)
set(RIDGEDASH_PLATFORM_LINK_LIBRARIES raylib)

if(APPLE)
    set_source_files_properties(
        "${RIDGEDASH_SRC_DIR}/platform/native_window_mac.mm"
        PROPERTIES LANGUAGE CXX COMPILE_OPTIONS "-x;objective-c++"
    )
    list(APPEND RIDGEDASH_PLATFORM_SOURCES
        "${RIDGEDASH_SRC_DIR}/platform/native_window_mac.mm"
    )
    list(APPEND RIDGEDASH_PLATFORM_LINK_LIBRARIES "-framework AppKit")
endif()

if(WIN32)
    set(RIDGEDASH_PLATFORM_WIN32_EXECUTABLE ON)
    list(APPEND RIDGEDASH_PLATFORM_LINK_LIBRARIES shell32)
endif()

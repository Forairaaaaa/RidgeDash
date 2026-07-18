/**
 * @file native_window_stub.cpp
 * @author Forairaaaaa
 * @brief Non-macOS fallback for native window hooks (uses raylib fullscreen).
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#if !defined(__APPLE__)

#include "platform/native_window.hpp"

#if !defined(RIDGEDASH_USE_PLATFORM_COMPAT)
#include <raylib.h>
#endif

namespace ridge_dash {

void enableNativeFullscreen() {}

void toggleNativeFullscreen()
{
#if !defined(RIDGEDASH_USE_PLATFORM_COMPAT)
    ToggleFullscreen();
#endif
}

bool isNativeFullscreen()
{
#if !defined(RIDGEDASH_USE_PLATFORM_COMPAT)
    return IsWindowFullscreen();
#else
    return false;
#endif
}

} // namespace ridge_dash

#endif // !__APPLE__

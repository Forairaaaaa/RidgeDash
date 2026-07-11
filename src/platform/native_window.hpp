/**
 * @file native_window.hpp
 * @author Forairaaaaa
 * @brief Platform hooks for native desktop window behavior (macOS fullscreen).
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

namespace ridge_dash {

// Make the OS window advertise native-fullscreen capability. On macOS this enables
// the green traffic-light button and the real fullscreen Space (Dock + menu bar
// auto-hide). No-op elsewhere.
void enableNativeFullscreen();

// Toggle native fullscreen. On macOS this drives the Cocoa fullscreen Space; on
// other desktops it falls back to raylib's ToggleFullscreen.
void toggleNativeFullscreen();

// Whether the window is currently in native fullscreen.
bool isNativeFullscreen();

} // namespace ridge_dash

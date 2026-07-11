/**
 * @file native_window_mac.mm
 * @author Forairaaaaa
 * @brief macOS native fullscreen via Cocoa (real fullscreen Space).
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "platform/native_window.hpp"

#import <AppKit/AppKit.h>

// raylib exposes the GLFW window; glfwGetCocoaWindow returns the NSWindow*.
extern "C" void* GetWindowHandle(void);

namespace ridge_dash {

namespace {

NSWindow* cocoaWindow()
{
    void* handle = GetWindowHandle();
    return handle ? (__bridge NSWindow*)handle : nil;
}

} // namespace

void enableNativeFullscreen()
{
    NSWindow* window = cocoaWindow();
    if (window == nil) {
        return;
    }
    // Allow the standard green/traffic-light fullscreen button and the fullscreen
    // Space (auto-hides Dock + menu bar), instead of GLFW's borderless behavior.
    window.collectionBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;
    window.styleMask |= NSWindowStyleMaskResizable;
}

void toggleNativeFullscreen()
{
    NSWindow* window = cocoaWindow();
    if (window == nil) {
        return;
    }
    [window toggleFullScreen:nil];
}

bool isNativeFullscreen()
{
    NSWindow* window = cocoaWindow();
    if (window == nil) {
        return false;
    }
    return (window.styleMask & NSWindowStyleMaskFullScreen) != 0;
}

} // namespace ridge_dash

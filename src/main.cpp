/**
 * @file main.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/ridge_dash_game.hpp"

#include "platform/native_window.hpp"
#include "platform/raylib_compat.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace {

constexpr int kScreenWidth = RIDGEDASH_SCREEN_WIDTH;
constexpr int kScreenHeight = RIDGEDASH_SCREEN_HEIGHT;

int renderScale()
{
    int scale = RidgeDashDefaultRenderScale();
    if (const char* value = std::getenv("RIDGEDASH_RENDER_SCALE")) {
        scale = std::atoi(value);
    }
    return std::clamp(scale, 1, 4);
}

#if defined(RIDGEDASH_DESKTOP_RENDER)
using DisplayScaleOption = ridge_dash::RidgeDashGame::DisplayScaleOption;

// Desktop render frame rate from RIDGEDASH_TARGET_FPS. "0"/"unlimited" uncaps the
// frame rate (SetTargetFPS(0)); any other value is clamped to a sane range. Physics
// stays fixed at 60Hz regardless — see RidgeDashGame::stepPhysics. Returns 0 for
// uncapped, otherwise the target fps.
int targetFps()
{
    if (const char* value = std::getenv("RIDGEDASH_TARGET_FPS")) {
        if (std::strcmp(value, "0") == 0 || std::strcmp(value, "unlimited") == 0) {
            return 0;
        }
        return std::clamp(std::atoi(value), 30, 240);
    }
    return 120;
}

DisplayScaleOption initialDisplayScaleOption()
{
    if (const char* value = std::getenv("RIDGEDASH_WINDOW_SCALE")) {
        if (std::strcmp(value, "full") == 0 || std::strcmp(value, "fullscreen") == 0) {
            return DisplayScaleOption::Fullscreen;
        }
        return static_cast<DisplayScaleOption>(std::clamp(std::atoi(value), 1, 4) - 1);
    }
    return DisplayScaleOption::Scale4;
}

// CRT post-process default from RIDGEDASH_CRT ("0"/"off" disables). Defaults on.
bool crtInitiallyEnabled()
{
    if (const char* value = std::getenv("RIDGEDASH_CRT")) {
        return !(std::strcmp(value, "0") == 0 || std::strcmp(value, "off") == 0);
    }
    return true;
}

int scaleValue(DisplayScaleOption option)
{
    switch (option) {
        case DisplayScaleOption::Scale1:
            return 1;
        case DisplayScaleOption::Scale2:
            return 2;
        case DisplayScaleOption::Scale3:
            return 3;
        case DisplayScaleOption::Scale4:
            return 4;
        case DisplayScaleOption::Fullscreen:
            return 1;
        default:
            return 3;
    }
}

// Fixed supersample factor for the render target, sized to the monitor so a
// fullscreen frame stays crisp. The FBO size is constant for the whole session;
// the window is just letterboxed onto it.
int fboSupersampleScale()
{
    const int monitor = GetCurrentMonitor();
    const int fit = std::max(1, static_cast<int>(std::ceil(static_cast<float>(GetMonitorHeight(monitor)) / kScreenHeight)));
    return std::clamp(std::max(renderScale(), fit), 1, 8);
}

// Apply a display option to the OS window. Fullscreen uses the platform's native
// fullscreen (real macOS fullscreen Space: hides the Dock and menu bar, greenlights
// the traffic-light button); scaled options exit fullscreen and size the window to
// an integer multiple. The frame is letterboxed onto the window every frame, so the
// exact window size no longer needs to be tracked here.
void applyDisplayOption(DisplayScaleOption option)
{
    if (option == DisplayScaleOption::Fullscreen) {
        if (!ridge_dash::isNativeFullscreen()) {
            ridge_dash::toggleNativeFullscreen();
        }
        return;
    }

    if (ridge_dash::isNativeFullscreen()) {
        ridge_dash::toggleNativeFullscreen();
    }
    const int scale = scaleValue(option);
    SetWindowSize(kScreenWidth * scale, kScreenHeight * scale);
}
#else
int windowScale()
{
    return 1;
}
#endif

} // namespace

int main()
{
#if defined(RIDGEDASH_DESKTOP_RENDER)
    DisplayScaleOption displayOption = initialDisplayScaleOption();
    const int displayScale = scaleValue(displayOption);
#else
    const int displayScale = windowScale();
#endif
    const int windowWidth = kScreenWidth * displayScale;
    const int windowHeight = kScreenHeight * displayScale;

#if defined(RIDGEDASH_DESKTOP_RENDER)
    // Resizable window so it can be freely dragged/zoomed; the frame is integer-scaled
    // and letterboxed to whatever size the window currently is. (No HIGHDPI: the game
    // is a fixed 320x170 target and the FBO already supersamples, so 1 window point =
    // 1 pixel keeps the letterbox math simple and avoids a quarter-frame blit.)
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
#else
    SetConfigFlags(FLAG_VSYNC_HINT);
#endif
    InitWindow(windowWidth, windowHeight, "RidgeDash");
    if (!IsWindowReady()) {
        CloseWindow();
        return 2;
    }
#if defined(RIDGEDASH_DESKTOP_RENDER)
    ridge_dash::enableNativeFullscreen();
    SetWindowMinSize(kScreenWidth, kScreenHeight);
#endif
#if defined(RIDGEDASH_ENABLE_AUDIO)
    InitAudioDevice();
#endif
#if !defined(RIDGEDASH_USE_FBDEV)
    SetExitKey(KEY_NULL);
#endif
#if defined(RIDGEDASH_DESKTOP_RENDER)
    const int desktopFps = targetFps();
    SetTargetFPS(desktopFps);
#else
    SetTargetFPS(60);
#endif
    RidgeDashInputInit();

#if defined(RIDGEDASH_DESKTOP_RENDER)
    applyDisplayOption(displayOption);
#else
    int targetScale = std::max(renderScale(), displayScale);
#endif

#if !defined(RIDGEDASH_USE_FBDEV)
    RenderTexture2D renderTarget{};
    auto unloadRenderTarget = [&]() {
        if (IsRenderTextureValid(renderTarget)) {
            UnloadRenderTexture(renderTarget);
            renderTarget = {};
        }
    };
    auto loadRenderTarget = [&](int scale) {
        unloadRenderTarget();
        if (scale <= 1) {
            return;
        }
        renderTarget = LoadRenderTexture(kScreenWidth * scale, kScreenHeight * scale);
        if (IsRenderTextureValid(renderTarget)) {
#if defined(RIDGEDASH_DESKTOP_RENDER)
            // Supersample: the target is rendered larger than the window and
            // filtered down bilinearly, smoothing geometry and sprite edges.
            SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_BILINEAR);
#else
            SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_POINT);
#endif
            SetTextureWrap(renderTarget.texture, TEXTURE_WRAP_CLAMP);
        }
    };
#if defined(RIDGEDASH_DESKTOP_RENDER)
    const int fboScale = fboSupersampleScale();
    loadRenderTarget(fboScale);
#else
    loadRenderTarget(targetScale);
#endif
#else
    const bool useRenderTarget = false;
#endif

#if defined(RIDGEDASH_DESKTOP_RENDER)
    // CRT post-process shader, applied when blitting the render target to the window.
    Shader crtShader{};
    int crtResolutionLoc = -1;
    int crtTimeLoc = -1;
    bool crtLoaded = false;
    const std::string crtAppDir = GetApplicationDirectory();
    const std::string crtCandidates[] = {
        "assets/shaders/crt.fs",
        crtAppDir + "assets/shaders/crt.fs",
        crtAppDir + "../assets/shaders/crt.fs",
        crtAppDir + "../Resources/assets/shaders/crt.fs", // macOS .app bundle
    };
    for (const std::string& path : crtCandidates) {
        if (FileExists(path.c_str())) {
            crtShader = LoadShader(nullptr, path.c_str());
            if (IsShaderValid(crtShader)) {
                crtResolutionLoc = GetShaderLocation(crtShader, "uResolution");
                crtTimeLoc = GetShaderLocation(crtShader, "uTime");
                crtLoaded = true;
            }
            break;
        }
    }
    bool crtEnabled = crtInitiallyEnabled() && crtLoaded;
#endif

    {
        ridge_dash::RidgeDashGame game;
#if defined(RIDGEDASH_DESKTOP_RENDER)
        game.setDisplayScaleOption(displayOption);
        game.setCrtEnabled(crtEnabled);
        bool cursorHiddenForFullscreen = false;
        // Interpolate rendering whenever the frame rate isn't locked to the physics
        // rate (uncapped or >60). At exactly 60 the live path renders identically.
        game.setInterpolationEnabled(desktopFps == 0 || desktopFps > 60);
#endif

        while (!RidgeDashWindowShouldClose() && !game.shouldQuit()) {
#if defined(RIDGEDASH_DESKTOP_RENDER)
            if (game.consumeDisplayScaleRequest(displayOption)) {
                applyDisplayOption(displayOption);
            }
            if (bool requested = false; game.consumeCrtRequest(requested)) {
                crtEnabled = requested && crtLoaded;
            }
            // Hide the cursor whenever the window is fullscreen (covers both the menu
            // and the OS green-button / shortcut), show it otherwise. Tracked so we
            // only toggle on change.
            if (const bool fs = ridge_dash::isNativeFullscreen(); fs != cursorHiddenForFullscreen) {
                if (fs) {
                    HideCursor();
                } else {
                    ShowCursor();
                }
                cursorHiddenForFullscreen = fs;
            }
#endif
            game.update(GetFrameTime());

#if !defined(RIDGEDASH_USE_FBDEV)
            if (IsRenderTextureValid(renderTarget)) {
#if defined(RIDGEDASH_DESKTOP_RENDER)
                // Letterbox the frame into the current window, integer-scaled and
                // centered, with black bars around it. Without HIGHDPI, screen size
                // (points) equals the framebuffer, so a plain blit fills the window.
                const int winW = GetScreenWidth();
                const int winH = GetScreenHeight();
                const int drawScale = std::max(1, std::min(winW / kScreenWidth, winH / kScreenHeight));
                const int destWidth = kScreenWidth * drawScale;
                const int destHeight = kScreenHeight * drawScale;
                const int destX = (winW - destWidth) / 2;
                const int destY = (winH - destHeight) / 2;
                const int renderScaleValue = fboScale;
#else
                const int destWidth = windowWidth;
                const int destHeight = windowHeight;
                const int destX = 0;
                const int destY = 0;
                const int renderScaleValue = targetScale;
#endif
                BeginTextureMode(renderTarget);
                ClearBackground(BLACK);
                BeginMode2D(Camera2D{{0.0f, 0.0f}, {0.0f, 0.0f}, 0.0f, static_cast<float>(renderScaleValue)});
                game.draw();
                EndMode2D();
                EndTextureMode();

                BeginDrawing();
                ClearBackground(BLACK);
#if defined(RIDGEDASH_DESKTOP_RENDER)
                if (crtEnabled) {
                    const Vector2 res = {static_cast<float>(destWidth), static_cast<float>(destHeight)};
                    const float t = static_cast<float>(GetTime());
                    SetShaderValue(crtShader, crtResolutionLoc, &res, SHADER_UNIFORM_VEC2);
                    SetShaderValue(crtShader, crtTimeLoc, &t, SHADER_UNIFORM_FLOAT);
                    BeginShaderMode(crtShader);
                }
#endif
                DrawTexturePro(renderTarget.texture,
                               Rectangle{0.0f,
                                         0.0f,
                                         static_cast<float>(renderTarget.texture.width),
                                         -static_cast<float>(renderTarget.texture.height)},
                               Rectangle{static_cast<float>(destX),
                                         static_cast<float>(destY),
                                         static_cast<float>(destWidth),
                                         static_cast<float>(destHeight)},
                               Vector2{0.0f, 0.0f},
                               0.0f,
                               WHITE);
#if defined(RIDGEDASH_DESKTOP_RENDER)
                if (crtEnabled) {
                    EndShaderMode();
                }
#endif
                EndDrawing();
                continue;
            }
#endif

            BeginDrawing();
#if !defined(RIDGEDASH_USE_FBDEV)
            if (displayScale > 1) {
                ClearBackground(BLACK);
                BeginMode2D(Camera2D{{0.0f, 0.0f}, {0.0f, 0.0f}, 0.0f, static_cast<float>(displayScale)});
                game.draw();
                EndMode2D();
            } else
#endif
            {
                game.draw();
            }
            EndDrawing();
        }
    }

#if !defined(RIDGEDASH_USE_FBDEV)
    unloadRenderTarget();
#endif
#if defined(RIDGEDASH_DESKTOP_RENDER)
    if (crtLoaded) {
        UnloadShader(crtShader);
    }
#endif

#if defined(RIDGEDASH_ENABLE_AUDIO)
    CloseAudioDevice();
#endif
    RidgeDashInputShutdown();
    CloseWindow();
    return 0;
}

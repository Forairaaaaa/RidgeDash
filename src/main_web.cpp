/**
 * @file main_web.cpp
 * @author Forairaaaaa
 * @brief WebAssembly (Emscripten) entry point for RidgeDash.
 *        Uses emscripten_set_main_loop instead of a blocking while-loop so the
 *        browser's JS thread is never blocked. All game state lives on the heap
 *        in WebState and persists across the per-frame callback.
 * @version 0.1
 * @date 2026-07-12
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <emscripten/emscripten.h>

#include "game/ridge_dash_game.hpp"
#include "platform/raylib_compat.hpp"

namespace {

constexpr int kScreenWidth  = RIDGEDASH_SCREEN_WIDTH;
constexpr int kScreenHeight = RIDGEDASH_SCREEN_HEIGHT;

// 2× logical scale → 640×340 WebGL canvas.  CSS in the shell will stretch it
// to fill the viewport while preserving the aspect ratio.
constexpr int kWebScale    = 2;
// 4× internal FBO → bilinear downsample to the 2× canvas = 4× SSAA.
// Matches the desktop's supersample approach (fboSupersampleScale renders at
// monitor-height / game-height, then bilinearly downscales to the window).
constexpr int kWebFboScale = 4;

struct WebState {
    ridge_dash::RidgeDashGame game;
    RenderTexture2D renderTarget{};
    Shader crtShader{};
    int crtResolutionLoc = -1;
    int crtTimeLoc       = -1;
    bool crtLoaded       = false;
    bool crtEnabled      = false; // user-toggled; starts true if shader loaded
};

WebState* gState = nullptr;

void webFrame()
{
    if (!gState || gState->game.shouldQuit()) {
        return;
    }

    gState->game.update(GetFrameTime());

    // Sync CRT toggle requested from the pause menu.
    if (bool requested = false; gState->game.consumeCrtRequest(requested)) {
        gState->crtEnabled = requested && gState->crtLoaded;
    }

    if (IsRenderTextureValid(gState->renderTarget)) {
        // Render the game at 4× (kWebFboScale) into the FBO, then bilinearly
        // downsample to the 2× canvas — equivalent to 4× SSAA.
        BeginTextureMode(gState->renderTarget);
        ClearBackground(BLACK);
        BeginMode2D(Camera2D{{0.0f, 0.0f}, {0.0f, 0.0f}, 0.0f, static_cast<float>(kWebFboScale)});
        gState->game.draw();
        EndMode2D();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        // Raylib render textures are bottom-up; negate height to flip the V axis.
        if (gState->crtEnabled) {
            const Vector2 res = {static_cast<float>(kScreenWidth * kWebScale),
                                 static_cast<float>(kScreenHeight * kWebScale)};
            const float t = static_cast<float>(GetTime());
            SetShaderValue(gState->crtShader, gState->crtResolutionLoc, &res, SHADER_UNIFORM_VEC2);
            SetShaderValue(gState->crtShader, gState->crtTimeLoc, &t, SHADER_UNIFORM_FLOAT);
            BeginShaderMode(gState->crtShader);
        }
        DrawTexturePro(
            gState->renderTarget.texture,
            Rectangle{0.0f, 0.0f,
                      static_cast<float>(gState->renderTarget.texture.width),
                      -static_cast<float>(gState->renderTarget.texture.height)},
            Rectangle{0.0f, 0.0f,
                      static_cast<float>(kScreenWidth * kWebScale),
                      static_cast<float>(kScreenHeight * kWebScale)},
            Vector2{0.0f, 0.0f},
            0.0f,
            WHITE);
        if (gState->crtEnabled) {
            EndShaderMode();
        }
        EndDrawing();
    } else {
        // Fallback: draw directly without a render target.
        BeginDrawing();
        ClearBackground(BLACK);
        gState->game.draw();
        EndDrawing();
    }
}

} // namespace

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(kScreenWidth * kWebScale, kScreenHeight * kWebScale, "RidgeDash");
    if (!IsWindowReady()) {
        return 2;
    }

#if defined(RIDGEDASH_ENABLE_AUDIO)
    InitAudioDevice();
#endif

    SetExitKey(KEY_NULL);
    // Emscripten drives frame timing via requestAnimationFrame; SetTargetFPS(0)
    // disables raylib's own sleep so emscripten_set_main_loop paces correctly.
    SetTargetFPS(0);
    RidgeDashInputInit();

    // Construct the game after InitWindow + InitAudioDevice so audio assets
    // are discovered during RidgeDashGame construction.
    gState = new WebState();

    // Enable render interpolation: the browser's requestAnimationFrame may run
    // at any rate (60/120/144 Hz), while physics is fixed at 60 Hz.  Without
    // interpolation the car position is rounded each frame and the smooth
    // camera offset makes it jump between adjacent pixels — visually a "ghost"
    // / double-image.  With interpolation the car is lerped between the two
    // most recent physics snapshots, giving smooth motion at any display rate.
    gState->game.setInterpolationEnabled(true);

    // FBO at 4× game resolution; bilinear filter is used when downsampling to
    // the 2× canvas, giving smooth anti-aliased edges on rotated sprites.
    gState->renderTarget =
        LoadRenderTexture(kScreenWidth * kWebFboScale, kScreenHeight * kWebFboScale);
    if (IsRenderTextureValid(gState->renderTarget)) {
        SetTextureFilter(gState->renderTarget.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(gState->renderTarget.texture, TEXTURE_WRAP_CLAMP);
    }

    // CRT post-process shader (GLSL ES 300 variant for WebGL2).
    // The shader file is preloaded into the virtual FS at assets/shaders/crt_es.fs.
    {
        Shader sh = LoadShader(nullptr, "assets/shaders/crt_es.fs");
        if (IsShaderValid(sh)) {
            gState->crtShader        = sh;
            gState->crtResolutionLoc = GetShaderLocation(sh, "uResolution");
            gState->crtTimeLoc       = GetShaderLocation(sh, "uTime");
            gState->crtLoaded        = true;
            gState->crtEnabled       = true;
            // Inform the game's pause menu about the initial CRT state.
            gState->game.setCrtEnabled(true);
        }
    }

    // fps=0 → use requestAnimationFrame.  simulate_infinite_loop=1 → never
    // return from main(); the browser owns the event loop from here.
    emscripten_set_main_loop(webFrame, 0, 1);
    return 0;
}

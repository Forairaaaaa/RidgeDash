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
constexpr int kWebScale = 2;

struct WebState {
    ridge_dash::RidgeDashGame game;
    RenderTexture2D renderTarget{};
};

WebState* gState = nullptr;

void webFrame()
{
    if (!gState || gState->game.shouldQuit()) {
        return;
    }

    gState->game.update(GetFrameTime());

    if (IsRenderTextureValid(gState->renderTarget)) {
        // Render the game at 2× into the FBO, then blit to the screen.
        BeginTextureMode(gState->renderTarget);
        ClearBackground(BLACK);
        BeginMode2D(Camera2D{{0.0f, 0.0f}, {0.0f, 0.0f}, 0.0f, static_cast<float>(kWebScale)});
        gState->game.draw();
        EndMode2D();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        // Raylib render textures are bottom-up; negate height to flip the V axis.
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

    gState->renderTarget =
        LoadRenderTexture(kScreenWidth * kWebScale, kScreenHeight * kWebScale);
    if (IsRenderTextureValid(gState->renderTarget)) {
        // Point filter keeps the pixel art sharp; bilinear would be blurry.
        SetTextureFilter(gState->renderTarget.texture, TEXTURE_FILTER_POINT);
        SetTextureWrap(gState->renderTarget.texture, TEXTURE_WRAP_CLAMP);
    }

    // fps=0 → use requestAnimationFrame.  simulate_infinite_loop=1 → never
    // return from main(); the browser owns the event loop from here.
    emscripten_set_main_loop(webFrame, 0, 1);
    return 0;
}

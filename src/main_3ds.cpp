#include "game/ridge_dash_game.hpp"
#include "platform/raylib_compat.hpp"

#include <core/hal/hal.hpp>

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(RIDGEDASH_SCREEN_WIDTH, RIDGEDASH_SCREEN_HEIGHT, "RidgeDash");
    if (!IsWindowReady()) {
        CloseWindow();
        return 2;
    }

    SetTargetFPS(60);
    RidgeDashInputInit();
    smooth_ui_toolkit::ui_hal::on_get_tick([]() {
        return RidgeDashPlatformTimeMs();
    });

    {
        ridge_dash::RidgeDashGame game;
        game.setBgmOn(false);
        game.setSfxOn(false);

        while (!game.shouldQuit() && !RidgeDashWindowShouldClose()) {
            game.update(GetFrameTime());
            BeginDrawing();
            game.draw();
            EndDrawing();
        }
    }

    RidgeDashInputShutdown();
    CloseWindow();
    return 0;
}

/**
 * @file game_input.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/ui/game_input.hpp"

#include "platform/raylib_compat.hpp"

namespace ridge_dash {
namespace {

bool downAny(int a, int b, int c, int d, int e, int f)
{
    return IsKeyDown(a) || IsKeyDown(b) || IsKeyDown(c) || IsKeyDown(d) || IsKeyDown(e) || IsKeyDown(f);
}

bool downAny(int a, int b, int c, int d, int e)
{
    return IsKeyDown(a) || IsKeyDown(b) || IsKeyDown(c) || IsKeyDown(d) || IsKeyDown(e);
}

bool pressedAny(int a, int b)
{
    return IsKeyPressed(a) || IsKeyPressed(b);
}

bool pressedAny(int a, int b, int c)
{
    return IsKeyPressed(a) || IsKeyPressed(b) || IsKeyPressed(c);
}

bool pressedAny(int a, int b, int c, int d)
{
    return IsKeyPressed(a) || IsKeyPressed(b) || IsKeyPressed(c) || IsKeyPressed(d);
}

} // namespace

void GameInput::poll()
{
    _drive.throttle = downAny(KEY_RIGHT, KEY_SPACE, KEY_D, KEY_C, KEY_X, KEY_SEVEN);
    _drive.brake = downAny(KEY_LEFT, KEY_A, KEY_Z, KEY_F, KEY_FIVE);

    _commands.pause = IsKeyPressed(KEY_ESCAPE);
    _commands.reset = IsKeyPressed(KEY_R);
    _commands.confirm = pressedAny(KEY_ENTER, KEY_E);
    _commands.squid = false;
    if (IsKeyPressed(KEY_G)) {
        ++_squidPressCount;
        if (_squidPressCount >= 3) {
            _commands.squid = true;
            _squidPressCount = 0;
        }
    }

    _menu.back = _commands.pause;
    _menu.confirm = _commands.confirm;
    _menu.up = pressedAny(KEY_UP, KEY_W, KEY_F);
    _menu.down = pressedAny(KEY_DOWN, KEY_S, KEY_X);
    _menu.left = pressedAny(KEY_LEFT, KEY_A, KEY_Z);
    _menu.right = pressedAny(KEY_RIGHT, KEY_D, KEY_C, KEY_SPACE);
}

const GameInput::Drive& GameInput::drive() const
{
    return _drive;
}

const GameInput::Commands& GameInput::commands() const
{
    return _commands;
}

const GameInput::Menu& GameInput::menu() const
{
    return _menu;
}

} // namespace ridge_dash

/**
 * @file pause_menu_controller.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/ui/pause_menu_controller.hpp"

#include "game/ui/game_ui.hpp"

namespace ridge_dash {

PauseMenuController::Action PauseMenuController::update(const GameInput::Menu& input, GameUi& ui)
{
    Action action = input.back ? Action::ExitPause : Action::None;

#if defined(RIDGEDASH_DESKTOP_RENDER)
    constexpr int kOptionCount = 2;
    if (input.up) {
        ui.setPauseSelection((ui.pauseSelection() + kOptionCount - 1) % kOptionCount);
    }
    if (input.down) {
        ui.setPauseSelection((ui.pauseSelection() + 1) % kOptionCount);
    }
    if (ui.pauseSelection() == 0 && input.left) {
        _displayScaleOption = nextDisplayScaleOption(_displayScaleOption, -1);
        _displayScaleRequestPending = true;
    }
    if (ui.pauseSelection() == 0 && input.right) {
        _displayScaleOption = nextDisplayScaleOption(_displayScaleOption, 1);
        _displayScaleRequestPending = true;
    }
    if (input.confirm && ui.pauseSelection() == 1) {
        action = Action::QuitGame;
    }
#else
    ui.setPauseSelection(0);
    if (input.confirm) {
        action = Action::QuitGame;
    }
#endif

    return action;
}

void PauseMenuController::setDisplayScaleOption(DisplayScaleOption option)
{
    _displayScaleOption = option;
}

bool PauseMenuController::consumeDisplayScaleRequest(DisplayScaleOption& option)
{
    if (!_displayScaleRequestPending) {
        return false;
    }

    _displayScaleRequestPending = false;
    option = _displayScaleOption;
    return true;
}

const char* PauseMenuController::scaleLabel() const
{
    return displayScaleLabel(_displayScaleOption);
}

} // namespace ridge_dash

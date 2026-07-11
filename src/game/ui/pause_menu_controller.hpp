/**
 * @file pause_menu_controller.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "game/ui/display_scale.hpp"
#include "game/ui/game_input.hpp"

namespace ridge_dash {

class GameUi;

class PauseMenuController {
public:
    enum class Action { None, ExitPause, QuitGame };

    Action update(const GameInput::Menu& input, GameUi& ui);

    void setDisplayScaleOption(DisplayScaleOption option);
    bool consumeDisplayScaleRequest(DisplayScaleOption& option);
    const char* scaleLabel() const;

    void setCrtEnabled(bool enabled);
    bool consumeCrtRequest(bool& enabled);
    bool crtEnabled() const;

private:
    DisplayScaleOption _displayScaleOption = DisplayScaleOption::Scale3;
    bool _displayScaleRequestPending = false;
    bool _crtEnabled = true;
    bool _crtRequestPending = false;
};

} // namespace ridge_dash

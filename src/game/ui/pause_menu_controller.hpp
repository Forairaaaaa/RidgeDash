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
    enum class MenuLevel { Main, Video, Audio };

    Action update(const GameInput::Menu& input, GameUi& ui);

    // Display scale.
    void setDisplayScaleOption(DisplayScaleOption option);
    DisplayScaleOption displayScaleOption() const;
    bool consumeDisplayScaleRequest(DisplayScaleOption& option);
    const char* scaleLabel() const;

    // CRT toggle.
    void setCrtEnabled(bool enabled);
    bool consumeCrtRequest(bool& enabled);
    bool crtEnabled() const;

    // Audio toggles.
    void setBgmOn(bool on);
    bool bgmOn() const;
    void setSfxOn(bool on);
    bool sfxOn() const;

    // Settings-dirty flag: true whenever any setting changed this frame.
    bool consumeSettingsChanged();

    // Current menu level for UI rendering.
    MenuLevel menuLevel() const;

private:
    DisplayScaleOption _displayScaleOption = DisplayScaleOption::Scale3;
    bool _displayScaleRequestPending = false;
    bool _crtEnabled = true;
    bool _crtRequestPending = false;
    bool _bgmOn = true;
    bool _sfxOn = true;
    bool _settingsDirty = false;
    MenuLevel _menuLevel = MenuLevel::Main;
};

} // namespace ridge_dash

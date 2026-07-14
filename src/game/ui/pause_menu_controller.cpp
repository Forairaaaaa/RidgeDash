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
    Action action = Action::None;

    // Back key: go up a level, or exit pause from main menu.
    if (input.back) {
        if (_menuLevel == MenuLevel::Main) {
            return Action::ExitPause;
        }
        _menuLevel = MenuLevel::Main;
        ui.setPauseSelection(0);
        return Action::None;
    }

#if defined(RIDGEDASH_DESKTOP_RENDER) || defined(RIDGEDASH_WEB)
    switch (_menuLevel) {
        case MenuLevel::Main: {
            constexpr int kItemCount = 3;
            if (input.up) {
                ui.setPauseSelection((ui.pauseSelection() + kItemCount - 1) % kItemCount);
            }
            if (input.down) {
                ui.setPauseSelection((ui.pauseSelection() + 1) % kItemCount);
            }
            if (input.confirm) {
                if (ui.pauseSelection() == 0) {
                    _menuLevel = MenuLevel::Video;
                    ui.setPauseSelection(0);
                } else if (ui.pauseSelection() == 1) {
                    _menuLevel = MenuLevel::Audio;
                    ui.setPauseSelection(0);
                } else {
                    action = Action::QuitGame;
                }
            }
            break;
        }

        case MenuLevel::Video: {
            constexpr int kItemCount = 2; // SCALE, CRT
            if (input.up) {
                ui.setPauseSelection((ui.pauseSelection() + kItemCount - 1) % kItemCount);
            }
            if (input.down) {
                ui.setPauseSelection((ui.pauseSelection() + 1) % kItemCount);
            }
            if (ui.pauseSelection() == 0) {
                if (input.left) {
                    _displayScaleOption = nextDisplayScaleOption(_displayScaleOption, -1);
                    _displayScaleRequestPending = true;
                    _settingsDirty = true;
                }
                if (input.right) {
                    _displayScaleOption = nextDisplayScaleOption(_displayScaleOption, 1);
                    _displayScaleRequestPending = true;
                    _settingsDirty = true;
                }
            }
            if (ui.pauseSelection() == 1 && (input.left || input.right || input.confirm)) {
                _crtEnabled = !_crtEnabled;
                _crtRequestPending = true;
                _settingsDirty = true;
            }
            break;
        }

        case MenuLevel::Audio: {
            constexpr int kItemCount = 2; // BGM, SFX
            if (input.up) {
                ui.setPauseSelection((ui.pauseSelection() + kItemCount - 1) % kItemCount);
            }
            if (input.down) {
                ui.setPauseSelection((ui.pauseSelection() + 1) % kItemCount);
            }
            if (ui.pauseSelection() == 0 && (input.left || input.right || input.confirm)) {
                _bgmOn = !_bgmOn;
                _settingsDirty = true;
            }
            if (ui.pauseSelection() == 1 && (input.left || input.right || input.confirm)) {
                _sfxOn = !_sfxOn;
                _settingsDirty = true;
            }
            break;
        }
    }
#elif defined(RIDGEDASH_DRM_RENDER)
    // DRM: no video settings, but audio menu is available.
    switch (_menuLevel) {
        case MenuLevel::Main: {
            constexpr int kItemCount = 2; // AUDIO, EXIT GAME
            if (input.up) {
                ui.setPauseSelection((ui.pauseSelection() + kItemCount - 1) % kItemCount);
            }
            if (input.down) {
                ui.setPauseSelection((ui.pauseSelection() + 1) % kItemCount);
            }
            if (input.confirm) {
                if (ui.pauseSelection() == 0) {
                    _menuLevel = MenuLevel::Audio;
                    ui.setPauseSelection(0);
                } else {
                    action = Action::QuitGame;
                }
            }
            break;
        }

        case MenuLevel::Audio: {
            constexpr int kItemCount = 2; // BGM, SFX
            if (input.up) {
                ui.setPauseSelection((ui.pauseSelection() + kItemCount - 1) % kItemCount);
            }
            if (input.down) {
                ui.setPauseSelection((ui.pauseSelection() + 1) % kItemCount);
            }
            if (ui.pauseSelection() == 0 && (input.left || input.right || input.confirm)) {
                _bgmOn = !_bgmOn;
                _settingsDirty = true;
            }
            if (ui.pauseSelection() == 1 && (input.left || input.right || input.confirm)) {
                _sfxOn = !_sfxOn;
                _settingsDirty = true;
            }
            break;
        }

        case MenuLevel::Video:
            break; // unreachable on DRM
    }
#else
    // FBDEV: simple exit only.
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

DisplayScaleOption PauseMenuController::displayScaleOption() const
{
    return _displayScaleOption;
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

void PauseMenuController::setCrtEnabled(bool enabled)
{
    _crtEnabled = enabled;
}

bool PauseMenuController::consumeCrtRequest(bool& enabled)
{
    if (!_crtRequestPending) {
        return false;
    }

    _crtRequestPending = false;
    enabled = _crtEnabled;
    return true;
}

bool PauseMenuController::crtEnabled() const
{
    return _crtEnabled;
}

const char* PauseMenuController::scaleLabel() const
{
    return displayScaleLabel(_displayScaleOption);
}

void PauseMenuController::setBgmOn(bool on)
{
    _bgmOn = on;
}

bool PauseMenuController::bgmOn() const
{
    return _bgmOn;
}

void PauseMenuController::setSfxOn(bool on)
{
    _sfxOn = on;
}

bool PauseMenuController::sfxOn() const
{
    return _sfxOn;
}

bool PauseMenuController::consumeSettingsChanged()
{
    const bool dirty = _settingsDirty;
    _settingsDirty = false;
    return dirty;
}

PauseMenuController::MenuLevel PauseMenuController::menuLevel() const
{
    return _menuLevel;
}

} // namespace ridge_dash

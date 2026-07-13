/**
 * @file game_ui.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "game/run/save_data.hpp"
#include "platform/raylib_compat.hpp"

#include <smooth_ui_toolkit.hpp>
#include <string>

namespace ridge_dash {

class GameUi {
public:
    struct HudView {
        Texture2D fuelCan{};
        float fuel = 0.0f;
        float fuelFlash = 0.0f;
        float distance = 0.0f;
    };

    struct RunSummaryView {
        float distance = 0.0f;
        int coins = 0;
        int flips = 0;
        bool newRecord = false;
        float timer = 0.0f;
    };

    struct PauseView {
        GameRecords records{};
        const char* scaleLabel = "";
        bool crtOn = true;
    };

    void configureAnimations();
    void resetRun();
    void updateAnimations(float dt, bool gameOver, float gameOverTimer);
    void updateScorePopup(float dt);
    void showScorePopup(int amount, const char* label);
    void triggerFuelCelebration();

    bool startTipsVisible() const;
    void startTipsExit();

    void enterPause();
    void startPauseExit();
    void completePauseExit();
    bool pauseExiting() const;
    bool pauseExitFinished() const;
    void updatePauseTimer(float dt);
    int pauseSelection() const;
    void setPauseSelection(int selection);

    void drawHud(const HudView& view) const;
    void drawStartTips() const;
    void drawGameOver(const RunSummaryView& view) const;
    void drawPauseMenu(const PauseView& view) const;

private:
    void resetStartTipsAnimation();
    void resetGameOverAnimation();
    void resetPauseAnimation();
    void startGameOverAnimation();

    smooth_ui_toolkit::AnimateValue _startTipsPanelY;
    smooth_ui_toolkit::AnimateValue _startTipsMaskAlpha;
    smooth_ui_toolkit::AnimateValue _gameOverPanelY;
    smooth_ui_toolkit::AnimateValue _gameOverMaskAlpha;
    smooth_ui_toolkit::AnimateValue _pausePanelY;
    smooth_ui_toolkit::AnimateValue _pauseMaskAlpha;
    float _startTipsPanelYValue = 0.0f;
    float _startTipsMaskAlphaValue = 0.0f;
    float _gameOverPanelYValue = 0.0f;
    float _gameOverMaskAlphaValue = 0.0f;
    float _pausePanelYValue = 0.0f;
    float _pauseMaskAlphaValue = 0.0f;
    float _scorePopupTimer = 0.0f;
    float _fuelCelebrationTimer = 0.0f;
    float _pauseTimer = 0.0f;
    int _scorePopupAmount = 0;
    int _pauseSelection = 0;
    std::string _scorePopupLabel;
    bool _showStartTips = true;
    bool _startTipsExiting = false;
    bool _gameOverPanelShown = false;
    bool _pauseExiting = false;
};

} // namespace ridge_dash

/**
 * @file render.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/ridge_dash_game.hpp"

#include "game/game_config.hpp"

#include <algorithm>

namespace ridge_dash {

using namespace game_config;

Vector2 RidgeDashGame::worldToScreen(b2Vec2 p) const
{
    return {p.x * kPixelsPerMeter - _camera.x, p.y * kPixelsPerMeter - _camera.y};
}

void RidgeDashGame::draw() const
{
    _environment.drawBackground(_camera, _runStats.distance());
    _terrain.draw(_camera);
    _pickups.draw(*this);
    drawDust();
    drawVehicle();
    _pickups.drawEffects(*this);
    _ui.drawHud(
        GameUi::HudView{_sprites.fuelCan, _runController.fuel(), _runController.fuelFlash(), _runStats.score()});
    if (_ui.startTipsVisible()) {
        _ui.drawStartTips();
    }
    if (_runController.paused()) {
        _ui.drawPauseMenu(GameUi::PauseView{_runRecords.records(), _pauseMenu.scaleLabel()});
    }
    if (_runController.gameOver() && _runController.gameOverTimer() >= 2.0f) {
        _ui.drawGameOver(GameUi::RunSummaryView{_runStats.score(),
                                                _runStats.coins(),
                                                _runStats.flips(),
                                                _runStats.newRecord(),
                                                std::max(0.0f, _runController.gameOverTimer() - 2.0f)});
    }
}

} // namespace ridge_dash

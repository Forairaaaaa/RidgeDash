/**
 * @file game_ui.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/ui/game_ui.hpp"

#include "game/game_config.hpp"
#include "game/render_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace ridge_dash {
namespace {

constexpr float kStartTipsPanelRestY = 39.0f;
constexpr float kStartTipsPanelHiddenY = 9.0f;
constexpr float kStartTipsPanelExitY = 63.0f;
constexpr float kGameOverPanelRestY = 40.0f;
constexpr float kGameOverPanelHiddenY = 20.0f;
constexpr float kPausePanelRestY = 36.0f;
constexpr float kPausePanelHiddenY = 16.0f;
constexpr float kPausePanelExitY = 60.0f;

int hudTextWidth(const char* text, int fontSize)
{
#if defined(RIDGEDASH_USE_FBDEV)
    const int scale = std::max(1, fontSize / 6);
    return static_cast<int>(std::strlen(text)) * 4 * scale;
#else
    return MeasureText(text, fontSize);
#endif
}

void drawTextRightAligned(const char* text, int rightX, int y, int fontSize, Color color, float rotation = 0.0f)
{
    if (rotation == 0.0f) {
        DrawText(text, rightX - hudTextWidth(text, fontSize), y, fontSize, color);
        return;
    }
    const Font font = GetFontDefault();
    const Vector2 size = MeasureTextEx(font, text, fontSize, 1.0f);
    const Vector2 center = {static_cast<float>(rightX) - size.x * 0.5f, static_cast<float>(y) + size.y * 0.5f};
    const Vector2 origin = {size.x * 0.5f, size.y * 0.5f};
    DrawTextPro(font, text, center, origin, rotation, fontSize, 1.0f, color);
}

void drawTextCentered(const char* text, int centerX, int y, int fontSize, Color color)
{
    DrawText(text, centerX - hudTextWidth(text, fontSize) / 2, y, fontSize, color);
}

float scoreStickerBob(float time)
{
    return std::sin(time * 2.4f) * 1.5f;
}

void drawRunSummarySticker(int rightX,
                           int y,
                           const char* scoreText,
                           int coins,
                           int flips,
                           bool newRecord,
                           float time,
                           float alpha)
{
    const char* detailText = TextFormat("COIN %d  FLIP %d", coins, flips);
    const char* recordText = "NEW RECORD";
    const Color recordColor = fadeColor(Color{255, 92, 86, 255}, 0.98f * alpha);
    const Color scoreColor = fadeColor(RAYWHITE, alpha);
    const Color detailColor = fadeColor(Color{255, 226, 91, 255}, 0.94f * alpha);
    const float bob = scoreStickerBob(time);

#if defined(RIDGEDASH_USE_FBDEV)
    const int iy = static_cast<int>(std::round(y + bob));
    drawTextRightAligned(scoreText, rightX, iy, 20, scoreColor);
    drawTextRightAligned(detailText, rightX, iy + 20, 16, detailColor);
    if (newRecord) {
        drawTextRightAligned(recordText, rightX, iy + 36, 16, recordColor);
    }
#else
    constexpr float kSpacing = 1.0f;
    const float pulse = 1.0f + std::sin(time * 3.3f) * 0.045f;
    const float rotation = 5.0f + std::sin(time * 1.7f) * 1.1f;
    const float recordSize = 16.0f * pulse;
    const float scoreSize = 20.0f * pulse;
    const float detailSize = 16.0f * pulse;
    const float scoreGap = 20.0f * pulse;
    const float recordGap = 16.0f * pulse;
    const Font font = GetFontDefault();
    const Vector2 recordSizePx = MeasureTextEx(font, recordText, recordSize, kSpacing);
    const Vector2 scoreSizePx = MeasureTextEx(font, scoreText, scoreSize, kSpacing);
    const Vector2 detailSizePx = MeasureTextEx(font, detailText, detailSize, kSpacing);
    const float lineY = static_cast<float>(y) + bob;

    DrawTextPro(font,
                scoreText,
                Vector2{static_cast<float>(rightX), lineY},
                Vector2{scoreSizePx.x, 0.0f},
                rotation,
                scoreSize,
                kSpacing,
                scoreColor);
    DrawTextPro(font,
                detailText,
                Vector2{static_cast<float>(rightX), lineY + scoreGap},
                Vector2{detailSizePx.x, 0.0f},
                rotation,
                detailSize,
                kSpacing,
                detailColor);
    if (newRecord) {
        DrawTextPro(font,
                    recordText,
                    Vector2{static_cast<float>(rightX), lineY + scoreGap + recordGap},
                    Vector2{recordSizePx.x, 0.0f},
                    rotation,
                    recordSize,
                    kSpacing,
                    recordColor);
    }
#endif
}

float clamp01(float value)
{
    return std::max(0.0f, std::min(value, 1.0f));
}

void drawPixelPanel(int x, int y, int width, int height, float alpha = 1.0f)
{
    alpha = clamp01(alpha);
    DrawRectangle(x + 4, y + 4, width, height, fadeColor(Color{0, 0, 0, 255}, 0.42f * alpha));
    DrawRectangle(x, y, width, height, fadeColor(Color{22, 29, 39, 255}, 0.96f * alpha));
    DrawRectangle(x + 2, y + 2, width - 4, height - 4, fadeColor(Color{44, 55, 67, 255}, 0.92f * alpha));
    DrawRectangle(x + 1, y + 1, width - 2, 1, fadeColor(Color{92, 112, 126, 255}, 0.86f * alpha));
    DrawRectangle(x + 1, y + height - 4, width - 2, 3, fadeColor(Color{11, 16, 24, 255}, 0.86f * alpha));
    DrawRectangle(x + width - 3, y + 2, 2, height - 6, fadeColor(Color{14, 19, 27, 255}, 0.74f * alpha));
}

} // namespace

using namespace game_config;

void GameUi::configureAnimations()
{
    auto setupSpring = [](smooth_ui_toolkit::AnimateValue& value, float visualDuration, float bounce) {
        auto& options = value.springOptions();
        options.visualDuration = visualDuration;
        options.bounce = bounce;
        options.restDelta = 0.04f;
        options.restSpeed = 0.04f;
    };
    auto setupEase = [](smooth_ui_toolkit::AnimateValue& value, float duration) {
        auto& options = value.easingOptions();
        options.duration = duration;
        options.easingFunction = smooth_ui_toolkit::ease::ease_out_quad;
    };

    setupSpring(_startTipsPanelY, 0.6f, 0.5f);
    setupSpring(_gameOverPanelY, 0.4f, 0.5f);
    setupSpring(_pausePanelY, 0.42f, 0.5f);
    setupEase(_startTipsMaskAlpha, 0.22f);
    setupEase(_gameOverMaskAlpha, 0.22f);
    setupEase(_pauseMaskAlpha, 0.18f);
}

void GameUi::resetRun()
{
    _scorePopupTimer = 0.0f;
    _fuelCelebrationTimer = 0.0f;
    _distanceCelebrationTimer = 0.0f;
    _distanceCelebrationCooldown = 0.0f;
    _scorePopupAmount = 0;
    _scorePopupLabel.clear();
    if (_showStartTips) {
        resetStartTipsAnimation();
    }
    resetGameOverAnimation();
    resetPauseAnimation();
    _pauseTimer = 0.0f;
    _pauseExiting = false;
}

void GameUi::updateAnimations(float dt, bool gameOver, float gameOverTimer)
{
    _startTipsPanelY.updateWithDelta(dt);
    _startTipsMaskAlpha.updateWithDelta(dt);
    _gameOverPanelY.updateWithDelta(dt);
    _gameOverMaskAlpha.updateWithDelta(dt);
    _pausePanelY.updateWithDelta(dt);
    _pauseMaskAlpha.updateWithDelta(dt);

    _startTipsPanelYValue = _startTipsPanelY.directValue();
    _startTipsMaskAlphaValue = _startTipsMaskAlpha.directValue();
    _gameOverPanelYValue = _gameOverPanelY.directValue();
    _gameOverMaskAlphaValue = _gameOverMaskAlpha.directValue();
    _pausePanelYValue = _pausePanelY.directValue();
    _pauseMaskAlphaValue = _pauseMaskAlpha.directValue();

    if (_showStartTips && _startTipsExiting && _startTipsMaskAlphaValue <= 0.02f) {
        _showStartTips = false;
        _startTipsExiting = false;
    }

    if (gameOver && gameOverTimer >= 2.0f && !_gameOverPanelShown) {
        startGameOverAnimation();
    }

    _fuelCelebrationTimer = std::max(0.0f, _fuelCelebrationTimer - dt);
    _distanceCelebrationTimer = std::max(0.0f, _distanceCelebrationTimer - dt);
    _distanceCelebrationCooldown = std::max(0.0f, _distanceCelebrationCooldown - dt);
}

void GameUi::updateScorePopup(float dt)
{
    _scorePopupTimer = std::max(0.0f, _scorePopupTimer - dt);
    if (_scorePopupTimer <= 0.0f) {
        _scorePopupAmount = 0;
        _scorePopupLabel.clear();
    }
}

void GameUi::showScorePopup(int amount, const char* label)
{
    const std::string nextLabel = label ? label : "";
    if (nextLabel != _scorePopupLabel) {
        _scorePopupAmount = 0;
    }
    _scorePopupAmount += amount;
    _scorePopupLabel = nextLabel;
    _scorePopupTimer = kScorePopupDuration;
    triggerDistanceCelebration(true);
}

void GameUi::triggerFuelCelebration()
{
    _fuelCelebrationTimer = 0.24f;
}

void GameUi::triggerDistanceCelebration(bool force)
{
    if (!force && _distanceCelebrationCooldown > 0.0f) {
        return;
    }
    _distanceCelebrationTimer = 0.24f;
    _distanceCelebrationCooldown = 0.42f;
}

bool GameUi::startTipsVisible() const
{
    return _showStartTips;
}

void GameUi::startTipsExit()
{
    _startTipsExiting = true;
    _startTipsPanelY.move(kStartTipsPanelExitY);
    _startTipsMaskAlpha.move(0.0f);
}

void GameUi::enterPause()
{
    _pauseExiting = false;
    _pauseTimer = 0.0f;
    _pauseSelection = 0;
    _pausePanelY.teleport(kPausePanelHiddenY);
    _pauseMaskAlpha.teleport(0.0f);
    _pausePanelY.move(kPausePanelRestY);
    _pauseMaskAlpha.move(1.0f);
    _pausePanelYValue = kPausePanelHiddenY;
    _pauseMaskAlphaValue = 0.0f;
}

void GameUi::startPauseExit()
{
    _pauseExiting = true;
    _pausePanelY.move(kPausePanelExitY);
    _pauseMaskAlpha.move(0.0f);
}

void GameUi::completePauseExit()
{
    _pauseExiting = false;
}

bool GameUi::pauseExiting() const
{
    return _pauseExiting;
}

bool GameUi::pauseExitFinished() const
{
    return _pauseExiting && _pauseMaskAlphaValue <= 0.02f;
}

void GameUi::updatePauseTimer(float dt)
{
    _pauseTimer += dt;
}

int GameUi::pauseSelection() const
{
    return _pauseSelection;
}

void GameUi::setPauseSelection(int selection)
{
    _pauseSelection = selection;
}

void GameUi::resetStartTipsAnimation()
{
    _startTipsExiting = false;
    _startTipsPanelY.teleport(kStartTipsPanelHiddenY);
    _startTipsMaskAlpha.teleport(0.0f);
    _startTipsPanelY.move(kStartTipsPanelRestY);
    _startTipsMaskAlpha.move(1.0f);
    _startTipsPanelYValue = kStartTipsPanelHiddenY;
    _startTipsMaskAlphaValue = 0.0f;
}

void GameUi::resetGameOverAnimation()
{
    _gameOverPanelShown = false;
    _gameOverPanelY.teleport(kGameOverPanelHiddenY);
    _gameOverMaskAlpha.teleport(0.0f);
    _gameOverPanelYValue = kGameOverPanelHiddenY;
    _gameOverMaskAlphaValue = 0.0f;
}

void GameUi::resetPauseAnimation()
{
    _pausePanelY.teleport(kPausePanelHiddenY);
    _pauseMaskAlpha.teleport(0.0f);
    _pausePanelYValue = kPausePanelHiddenY;
    _pauseMaskAlphaValue = 0.0f;
}

void GameUi::startGameOverAnimation()
{
    _gameOverPanelShown = true;
    _gameOverPanelY.move(kGameOverPanelRestY);
    _gameOverMaskAlpha.move(1.0f);
}

void GameUi::drawHud(const HudView& view) const
{
    constexpr int kIconX = 8;
    constexpr int kIconY = kScreenHeight - 24;
    constexpr int kBarX = 31;
    constexpr int kBarY = kScreenHeight - 20;
    constexpr int kBarWidth = 32;
    constexpr int kBarHeight = 10;

    // Fuel celebration: Balatro-style scale pop + rotation shake that decays.
    float celebScale = 1.0f;
    float celebAngle = 0.0f;
    if (_fuelCelebrationTimer > 0.0f) {
        constexpr float kCelebDuration = 0.24f;
        const float t = 1.0f - (_fuelCelebrationTimer / kCelebDuration);
        const float decay = 1.0f - t;
        celebScale = 1.0f + 0.32f * std::sin(t * 3.14159f) * decay;
        celebAngle = 11.0f * std::sin(t * 16.0f) * decay;
    }

    if (textureLoaded(view.fuelCan)) {
        const float iconCX = 16.5f;
        const float iconCY = static_cast<float>(kIconY + 8.5f);
        const float iconW = 15.0f * celebScale;
        const float iconH = 19.0f * celebScale;
        drawSpriteCentered(view.fuelCan,
                           Vector2{iconCX + 2.0f, iconCY + 2.0f},
                           iconW,
                           iconH,
                           celebAngle,
                           fadeColor(BLACK, 0.20f));
        drawSpriteCentered(view.fuelCan, Vector2{iconCX, iconCY}, iconW, iconH, celebAngle);
    } else {
        DrawRectangle(kIconX + 2, kIconY + 5, 12, 14, fadeColor(BLACK, 0.20f));
        DrawRectangle(kIconX + 6, kIconY, 7, 5, fadeColor(BLACK, 0.20f));
        DrawRectangle(kIconX + 10, kIconY + 9, 3, 7, fadeColor(BLACK, 0.20f));
        DrawRectangle(kIconX, kIconY + 3, 12, 14, Color{196, 48, 58, 255});
        DrawRectangle(kIconX + 4, kIconY - 2, 7, 5, Color{242, 202, 95, 255});
        DrawRectangle(kIconX + 8, kIconY + 7, 3, 7, Color{241, 100, 70, 255});
    }

    const int fuelWidth = static_cast<int>(kBarWidth * (view.fuel / kMaxFuel));
    const Color fuelColor = view.fuel > 25.0f ? Color{131, 255, 160, 255} : Color{230, 78, 68, 255};
    DrawRectangle(kBarX + 2, kBarY + 2, kBarWidth, kBarHeight, fadeColor(Color{0, 0, 0, 255}, 0.20f));
    DrawRectangle(kBarX - 1, kBarY - 1, kBarWidth + 2, kBarHeight + 2, Color{21, 28, 35, 230});
    DrawRectangle(kBarX, kBarY, kBarWidth, kBarHeight, Color{47, 57, 63, 230});
    DrawRectangle(kBarX, kBarY, fuelWidth, kBarHeight, fuelColor);
    DrawRectangle(kBarX, kBarY + kBarHeight - 2, kBarWidth, 2, fadeColor(Color{9, 14, 20, 255}, 0.30f));
    if (view.fuelFlash > 0.0f) {
        DrawRectangleLines(kBarX - 1,
                           kBarY - 1,
                           kBarWidth + 2,
                           kBarHeight + 2,
                           fadeColor(Color{255, 238, 156, 255}, view.fuelFlash / 0.24f));
    }
    const char* distanceText = TextFormat("%dm", static_cast<int>(view.distance));
    if (_scorePopupAmount > 0 && _scorePopupTimer > 0.0f) {
        const float t = 1.0f - (_scorePopupTimer / kScorePopupDuration);
        const float fade = std::min(1.0f, _scorePopupTimer / 0.24f);
        const char* popupText = _scorePopupLabel.empty()
                                    ? TextFormat("+%d", _scorePopupAmount)
                                    : TextFormat("%s +%d", _scorePopupLabel.c_str(), _scorePopupAmount);
        drawTextRightAligned(popupText,
                             kScreenWidth - 8,
                             static_cast<int>(kScreenHeight - 28 - t * 6.0f),
                             10,
                             fadeColor(Color{255, 226, 91, 255}, fade));
    }
    // Distance celebration: same scale + rotation pop as the fuel icon.
    float distScale = 1.0f;
    float distAngle = 0.0f;
    if (_distanceCelebrationTimer > 0.0f) {
        constexpr float kCelebDuration = 0.24f;
        const float t = 1.0f - (_distanceCelebrationTimer / kCelebDuration);
        const float decay = 1.0f - t;
        distScale = 1.0f + 0.32f * std::sin(t * 3.14159f) * decay;
        distAngle = 8.0f * std::sin(t * 16.0f) * decay;
    }
    const int distX = kScreenWidth - 8;
    const int distY = kScreenHeight - 13;
    const int fontSize = static_cast<int>(10.0f * distScale);
    drawTextRightAligned(distanceText, distX, distY, fontSize, RAYWHITE, distAngle);
}

void GameUi::drawStartTips() const
{
    constexpr int kPanelW = 236;
    constexpr int kPanelH = 58;
    constexpr int kPanelX = (kScreenWidth - kPanelW) / 2;
    const float alpha = clamp01(_startTipsMaskAlphaValue);
    const int y = static_cast<int>(std::round(_startTipsPanelYValue));

    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, fadeColor(Color{0, 0, 0, 255}, 0.30f * alpha));
    drawPixelPanel(kPanelX, y, kPanelW, kPanelH, alpha);

    drawTextCentered("[LEFT]  BRAKE", kScreenWidth / 2, y + 11, 10, fadeColor(Color{255, 239, 186, 255}, alpha));
    drawTextCentered("[RIGHT / SPACE]  GAS", kScreenWidth / 2, y + 26, 10, fadeColor(Color{131, 255, 160, 255}, alpha));
    drawTextCentered("[R]  RESET", kScreenWidth / 2, y + 41, 10, fadeColor(Color{220, 230, 238, 255}, 0.92f * alpha));
}

void GameUi::drawGameOver(const RunSummaryView& view) const
{
    constexpr int kPanelW = 212;
    constexpr int kPanelH = 78;
    constexpr int kPanelX = (kScreenWidth - kPanelW) / 2;
    const float alpha = clamp01(_gameOverMaskAlphaValue);
    const int y = static_cast<int>(std::round(_gameOverPanelYValue));

    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, fadeColor(BLACK, 0.58f * alpha));
    drawPixelPanel(kPanelX, y, kPanelW, kPanelH, alpha);
    DrawText("RUN OVER", kPanelX + 13, y + 12, 14, fadeColor(Color{223, 186, 255, 255}, alpha));
    drawRunSummarySticker(kPanelX + kPanelW - 7 + 10,
                          y + 13 - 20,
                          TextFormat("%dm", static_cast<int>(view.distance)),
                          view.coins,
                          view.flips,
                          view.newRecord,
                          view.timer,
                          alpha);
    drawTextCentered(
        "[ENTER / R]  RETRY", kScreenWidth / 2, y + 60 - 7, 10, fadeColor(Color{220, 230, 238, 255}, 0.92f * alpha));
}

void GameUi::drawPauseMenu(const PauseView& view) const
{
#if defined(RIDGEDASH_DESKTOP_RENDER)
    constexpr int kPanelW = 226;
    constexpr int kPanelH = 98;
#elif defined(RIDGEDASH_WEB)
    constexpr int kPanelW = 202;
    constexpr int kPanelH = 80;
#else
    constexpr int kPanelW = 202;
    constexpr int kPanelH = 66;
#endif
    constexpr int kPanelX = (kScreenWidth - kPanelW) / 2;
    const float alpha = clamp01(_pauseMaskAlphaValue);
    const int y = static_cast<int>(std::round(_pausePanelYValue));

    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, fadeColor(BLACK, 0.44f * alpha));
    drawPixelPanel(kPanelX, y, kPanelW, kPanelH, alpha);
    DrawText("PAUSED", kPanelX + 13, y + 11, 14, fadeColor(Color{223, 186, 255, 255}, alpha));

    drawRunSummarySticker(kPanelX + kPanelW - 12 + 5,
                          y + 10 - 10,
                          TextFormat("BEST: %dm", view.records.score),
                          view.records.coins,
                          view.records.flips,
                          false,
                          _pauseTimer,
                          alpha);

#if defined(RIDGEDASH_DESKTOP_RENDER)
    const Color selected = fadeColor(Color{131, 255, 160, 255}, alpha);
    const Color normal = fadeColor(Color{220, 230, 238, 255}, 0.86f * alpha);
    DrawText(_pauseSelection == 0 ? ">" : " ", kPanelX + 16, y + 47, 10, _pauseSelection == 0 ? selected : normal);
    DrawText("SCALE", kPanelX + 28, y + 47, 10, _pauseSelection == 0 ? selected : normal);
    drawTextRightAligned(TextFormat("< %s >", view.scaleLabel),
                         kPanelX + kPanelW - 18,
                         y + 47,
                         10,
                         _pauseSelection == 0 ? selected : normal);

    DrawText(_pauseSelection == 1 ? ">" : " ", kPanelX + 16, y + 62, 10, _pauseSelection == 1 ? selected : normal);
    DrawText("CRT", kPanelX + 28, y + 62, 10, _pauseSelection == 1 ? selected : normal);
    drawTextRightAligned(TextFormat("< %s >", view.crtOn ? "ON" : "OFF"),
                         kPanelX + kPanelW - 18,
                         y + 62,
                         10,
                         _pauseSelection == 1 ? selected : normal);

    DrawText(_pauseSelection == 2 ? ">" : " ", kPanelX + 16, y + 77, 10, _pauseSelection == 2 ? selected : normal);
    DrawText("EXIT", kPanelX + 28, y + 77, 10, _pauseSelection == 2 ? selected : normal);
#elif defined(RIDGEDASH_WEB)
    {
        const Color sel = fadeColor(Color{131, 255, 160, 255}, alpha);
        const Color nor = fadeColor(Color{220, 230, 238, 255}, 0.86f * alpha);
        DrawText(_pauseSelection == 0 ? ">" : " ", kPanelX + 16, y + 47, 10, _pauseSelection == 0 ? sel : nor);
        DrawText("CRT", kPanelX + 28, y + 47, 10, _pauseSelection == 0 ? sel : nor);
        drawTextRightAligned(TextFormat("< %s >", view.crtOn ? "ON" : "OFF"),
                             kPanelX + kPanelW - 18, y + 47, 10, _pauseSelection == 0 ? sel : nor);
        DrawText(_pauseSelection == 1 ? ">" : " ", kPanelX + 16, y + 62, 10, _pauseSelection == 1 ? sel : nor);
        DrawText("EXIT", kPanelX + 28, y + 62, 10, _pauseSelection == 1 ? sel : nor);
    }
#else
    DrawText(">", kPanelX + 18, y + 45, 10, fadeColor(Color{131, 255, 160, 255}, alpha));
    DrawText("EXIT", kPanelX + 30, y + 45, 10, fadeColor(Color{131, 255, 160, 255}, alpha));
#endif
}

} // namespace ridge_dash

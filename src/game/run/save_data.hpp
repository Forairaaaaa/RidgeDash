/**
 * @file save_data.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

namespace ridge_dash {

struct GameRecords {
    int score = 0;
    int coins = 0;
    int flips = 0;
    int frontFlips = 0;
    int backFlips = 0;
};

GameRecords loadGameRecords();
void saveGameRecords(const GameRecords& records);

struct GameSettings {
    int displayScale = 3; // DisplayScaleOption enum value (0=1X .. 4=Fullscreen)
    bool crtEnabled = true;
    bool bgmOn = true;
    bool sfxOn = true;
};

GameSettings loadGameSettings();
void saveGameSettings(const GameSettings& settings);

} // namespace ridge_dash

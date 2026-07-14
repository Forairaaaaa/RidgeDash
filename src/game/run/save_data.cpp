/**
 * @file save_data.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/run/save_data.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace ridge_dash {
namespace {

std::filesystem::path stateDir()
{
    if (const char* stateHome = std::getenv("XDG_STATE_HOME")) {
        if (stateHome[0] != '\0') {
            return std::filesystem::path(stateHome) / "ridgedash";
        }
    }
    if (const char* home = std::getenv("HOME")) {
        if (home[0] != '\0') {
            return std::filesystem::path(home) / ".local" / "state" / "ridgedash";
        }
    }
    return std::filesystem::path(".");
}

std::filesystem::path recordsPath()
{
    return stateDir() / "records.txt";
}

std::filesystem::path settingsPath()
{
    return stateDir() / "settings.txt";
}

int clampRecordValue(int value)
{
    return std::max(0, std::min(value, 9999999));
}

} // namespace

GameRecords loadGameRecords()
{
    GameRecords records{};
    std::ifstream file(recordsPath());
    if (!file) {
        return records;
    }

    std::string key;
    int value = 0;
    while (file >> key >> value) {
        if (key == "score") {
            records.score = clampRecordValue(value);
        } else if (key == "coins") {
            records.coins = clampRecordValue(value);
        } else if (key == "flips") {
            records.flips = clampRecordValue(value);
        } else if (key == "front_flips") {
            records.frontFlips = clampRecordValue(value);
        } else if (key == "back_flips") {
            records.backFlips = clampRecordValue(value);
        }
    }
    return records;
}

void saveGameRecords(const GameRecords& records)
{
    const std::filesystem::path path = recordsPath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        return;
    }
    file << "score " << clampRecordValue(records.score) << '\n';
    file << "coins " << clampRecordValue(records.coins) << '\n';
    file << "flips " << clampRecordValue(records.flips) << '\n';
    file << "front_flips " << clampRecordValue(records.frontFlips) << '\n';
    file << "back_flips " << clampRecordValue(records.backFlips) << '\n';
}

GameSettings loadGameSettings()
{
    GameSettings settings{};
    std::ifstream file(settingsPath());
    if (!file) {
        return settings;
    }

    std::string key;
    int value = 0;
    while (file >> key >> value) {
        if (key == "display_scale") {
            settings.displayScale = std::clamp(value, 0, 4);
        } else if (key == "crt") {
            settings.crtEnabled = (value != 0);
        }
    }
    return settings;
}

void saveGameSettings(const GameSettings& settings)
{
    const std::filesystem::path path = settingsPath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        return;
    }
    file << "display_scale " << settings.displayScale << '\n';
    file << "crt " << (settings.crtEnabled ? 1 : 0) << '\n';
}

} // namespace ridge_dash

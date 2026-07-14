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

std::filesystem::path recordsPath()
{
    if (const char* stateHome = std::getenv("XDG_STATE_HOME")) {
        if (stateHome[0] != '\0') {
            return std::filesystem::path(stateHome) / "ridgedash" / "records.txt";
        }
    }
    if (const char* home = std::getenv("HOME")) {
        if (home[0] != '\0') {
            return std::filesystem::path(home) / ".local" / "state" / "ridgedash" / "records.txt";
        }
    }
    return std::filesystem::path("ridgedash-records.txt");
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

} // namespace ridge_dash

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
#include <fstream>
#include <string>

#if defined(RIDGEDASH_3DS)
#include <sys/stat.h>
#else
#include <filesystem>
#endif

#if defined(_WIN32)
#include <shlobj.h>
#endif

namespace ridge_dash {
namespace {

#if defined(RIDGEDASH_3DS)
using SavePath = std::string;

SavePath stateDir()
{
    return "sdmc:/3ds/RidgeDash";
}

void ensureStateDir()
{
    mkdir("sdmc:/3ds", 0777);
    mkdir(stateDir().c_str(), 0777);
}
#else
using SavePath = std::filesystem::path;

SavePath stateDir()
{
#if defined(_WIN32)
    wchar_t localAppData[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(
            nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, localAppData))) {
        return std::filesystem::path(localAppData) / L"RidgeDash";
    }

    // HOME is normally not defined for applications launched from Windows
    // Explorer. Keep environment lookups as a fallback for unusual shells.
    if (const char* localAppData = std::getenv("LOCALAPPDATA")) {
        if (localAppData[0] != '\0') {
            return std::filesystem::path(localAppData) / "RidgeDash";
        }
    }
    if (const char* appData = std::getenv("APPDATA")) {
        if (appData[0] != '\0') {
            return std::filesystem::path(appData) / "RidgeDash";
        }
    }
#endif

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

void ensureStateDir()
{
    std::error_code ec;
    std::filesystem::create_directories(stateDir(), ec);
}
#endif

SavePath recordsPath()
{
#if defined(RIDGEDASH_3DS)
    return stateDir() + "/records.txt";
#else
    return stateDir() / "records.txt";
#endif
}

SavePath settingsPath()
{
#if defined(RIDGEDASH_3DS)
    return stateDir() + "/settings.txt";
#else
    return stateDir() / "settings.txt";
#endif
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
    const SavePath path = recordsPath();
    ensureStateDir();

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
        } else if (key == "bgm") {
            settings.bgmOn = (value != 0);
        } else if (key == "sfx") {
            settings.sfxOn = (value != 0);
        }
    }
    return settings;
}

void saveGameSettings(const GameSettings& settings)
{
    const SavePath path = settingsPath();
    ensureStateDir();

    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        return;
    }
    file << "display_scale " << settings.displayScale << '\n';
    file << "crt " << (settings.crtEnabled ? 1 : 0) << '\n';
    file << "bgm " << (settings.bgmOn ? 1 : 0) << '\n';
    file << "sfx " << (settings.sfxOn ? 1 : 0) << '\n';
}

} // namespace ridge_dash

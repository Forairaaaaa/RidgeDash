/**
 * @file run_records.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "game/run/run_stats.hpp"
#include "game/run/save_data.hpp"

namespace ridge_dash {

class RunRecords {
public:
    RunRecords() : _records(loadGameRecords()) {}

    const GameRecords& records() const
    {
        return _records;
    }

    bool submit(RunStats& stats)
    {
        if (stats.roundedScore() <= _records.score) {
            return false;
        }

        _records = GameRecords{stats.roundedScore(), stats.coins(), stats.flips()};
        stats.markNewRecord();
        saveGameRecords(_records);
        return true;
    }

private:
    GameRecords _records{};
};

} // namespace ridge_dash

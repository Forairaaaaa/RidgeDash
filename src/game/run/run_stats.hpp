/**
 * @file run_stats.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <algorithm>
#include <cmath>

namespace ridge_dash {

class RunStats {
public:
    void reset()
    {
        _distance = 0.0f;
        _coinScore = 0.0f;
        _trickScore = 0.0f;
        _score = 0.0f;
        _coins = 0;
        _flips = 0;
        _newRecord = false;
    }

    void updateDistance(float distance)
    {
        _distance = std::max(_distance, distance);
        updateScore();
    }

    void addCoin(float score)
    {
        ++_coins;
        _coinScore += score;
        updateScore();
    }

    void addFlipBonus(int flips, int score)
    {
        _flips += flips;
        _trickScore += static_cast<float>(score);
        updateScore();
    }

    float distance() const
    {
        return _distance;
    }

    float score() const
    {
        return _score;
    }

    int roundedScore() const
    {
        return static_cast<int>(std::round(_score));
    }

    int coins() const
    {
        return _coins;
    }

    int flips() const
    {
        return _flips;
    }

    bool newRecord() const
    {
        return _newRecord;
    }

    void markNewRecord()
    {
        _newRecord = true;
    }

private:
    void updateScore()
    {
        _score = std::max(_score, _distance + _coinScore + _trickScore);
    }

    float _distance = 0.0f;
    float _coinScore = 0.0f;
    float _trickScore = 0.0f;
    float _score = 0.0f;
    int _coins = 0;
    int _flips = 0;
    bool _newRecord = false;
};

} // namespace ridge_dash

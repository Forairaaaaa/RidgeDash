/**
 * @file trick_tracker.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/run/trick_tracker.hpp"

#include "game/game_config.hpp"

#include <cmath>

namespace ridge_dash {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = kPi * 2.0f;

float wrapAngleDelta(float delta)
{
    while (delta > kPi) {
        delta -= kTwoPi;
    }
    while (delta < -kPi) {
        delta += kTwoPi;
    }
    return delta;
}

// Number of full flips represented by an accumulated absolute rotation. Matches
// the landing scoring: first flip at kFlipBonusAngle, then one per full turn.
int flipCountFor(float flipAmount)
{
    return flipAmount >= game_config::kFlipBonusAngle
               ? 1 + static_cast<int>((flipAmount - game_config::kFlipBonusAngle) / kTwoPi)
               : 0;
}

} // namespace

using namespace game_config;

void TrickTracker::reset()
{
    _airTime = 0.0f;
    _flipAngle = 0.0f;
    _lastFlipAngle = 0.0f;
    _announcedFlips = 0;
    _wasAirborne = false;
}

TrickTracker::Bonus TrickTracker::update(float dt, const Sample& sample)
{
    if (sample.blocked) {
        reset();
        return {};
    }

    const bool airborne = !sample.rearGrounded && !sample.frontGrounded;
    const bool rolling = sample.angularSpeed > 0.85f
                         || (_wasAirborne && sample.angularSpeed > 0.12f && std::abs(_flipAngle) > 0.10f);
    const bool tracking = airborne || rolling;

    if (tracking) {
        if (!_wasAirborne) {
            _airTime = 0.0f;
            _flipAngle = 0.0f;
            _announcedFlips = 0;
            _lastFlipAngle = sample.angle;
        } else {
            _flipAngle += wrapAngleDelta(sample.angle - _lastFlipAngle);
        }
        _lastFlipAngle = sample.angle;
        _airTime += dt;
        _wasAirborne = true;

        // Credit each flip the moment it is recognised, mid-air, so the score,
        // popup and sound fire immediately and keep counting up (FLIP, 2x FLIP, ...).
        Bonus bonus{};
        const int flips = flipCountFor(std::abs(_flipAngle));
        if (flips > _announcedFlips) {
            _announcedFlips = flips;
            bonus.newFlip = true;
            bonus.flipIndex = flips;
            bonus.frontFlip = _flipAngle > 0.0f;
        }
        return bonus;
    }

    // Catch a flip that completed on the exact touchdown frame.
    Bonus bonus{};
    if (_wasAirborne && _airTime > 0.10f) {
        const int flips = flipCountFor(std::abs(_flipAngle));
        if (flips > _announcedFlips) {
            bonus.newFlip = true;
            bonus.flipIndex = flips;
            bonus.frontFlip = _flipAngle > 0.0f;
        }
    }

    _wasAirborne = false;
    _airTime = 0.0f;
    _flipAngle = 0.0f;
    _announcedFlips = 0;
    _lastFlipAngle = sample.angle;
    return bonus;
}

} // namespace ridge_dash

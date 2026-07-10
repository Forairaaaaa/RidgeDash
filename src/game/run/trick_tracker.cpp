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

} // namespace

using namespace game_config;

void TrickTracker::reset()
{
    _airTime = 0.0f;
    _flipAngle = 0.0f;
    _lastFlipAngle = 0.0f;
    _wasAirborne = false;
}

TrickTracker::Bonus TrickTracker::update(float dt, const Sample& sample)
{
    if (sample.blocked) {
        reset();
        return {};
    }

    const bool airborne = !sample.rearGrounded && !sample.frontGrounded;
    const bool rolling =
        sample.angularSpeed > 1.25f || (_wasAirborne && sample.angularSpeed > 0.18f && std::abs(_flipAngle) > 0.24f);
    const bool tracking = airborne || rolling;

    if (tracking) {
        if (!_wasAirborne) {
            _airTime = 0.0f;
            _flipAngle = 0.0f;
            _lastFlipAngle = sample.angle;
        } else {
            _flipAngle += wrapAngleDelta(sample.angle - _lastFlipAngle);
        }
        _lastFlipAngle = sample.angle;
        _airTime += dt;
        _wasAirborne = true;
        return {};
    }

    Bonus bonus{};
    if (_wasAirborne && _airTime > 0.10f) {
        const float flipAmount = std::abs(_flipAngle);
        bonus.flips = flipAmount >= kFlipBonusAngle ? 1 + static_cast<int>((flipAmount - kFlipBonusAngle) / kTwoPi) : 0;
        bonus.score = bonus.flips * kFlipBonusScore;
    }

    _wasAirborne = false;
    _airTime = 0.0f;
    _flipAngle = 0.0f;
    _lastFlipAngle = sample.angle;
    return bonus;
}

} // namespace ridge_dash

/**
 * @file game_config.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <algorithm>
#include <cmath>

#include <box2d/box2d.h>

namespace ridge_dash::game_config {

constexpr int kScreenWidth = RIDGEDASH_SCREEN_WIDTH;
constexpr int kScreenHeight = RIDGEDASH_SCREEN_HEIGHT;

constexpr float kPixelsPerMeter = 14.0f;
constexpr float kPhysicsStep = 1.0f / 60.0f;
constexpr float kMaxFrameDt = 1.0f / 20.0f;
constexpr float kTerrainStep = 2.85f;
constexpr float kTerrainStartX = -12.0f;
constexpr float kTerrainFlatUntilX = 20.0f;
constexpr float kTerrainGenerateAhead = 280.0f;
constexpr float kTerrainKeepBehind = 90.0f;
constexpr float kFuelGenerateAhead = 340.0f;
constexpr float kCoinGenerateAhead = 320.0f;
constexpr float kFleaGenerateAhead = 330.0f;
constexpr float kRocketGenerateAhead = 360.0f;
constexpr float kCactusGenerateAhead = 330.0f;
constexpr float kSnowmanGenerateAhead = 330.0f;
constexpr float kGroundThickness = 0.55f;
constexpr float kWheelRadius = 0.42f;
constexpr float kFuelCanRadius = 0.42f;
constexpr float kCoinRadius = 0.38f;
constexpr float kFleaRadius = 0.44f;
constexpr float kRocketRadius = 0.62f;
constexpr float kCactusRadius = 0.72f;
constexpr float kSnowmanRadius = 0.64f;
constexpr float kFuelPickupDistance = 1.08f;
constexpr float kCoinPickupDistance = 0.96f;
constexpr float kFleaPickupDistance = 1.05f;
constexpr float kRocketPickupDistance = 1.16f;
constexpr float kCactusPickupDistance = 1.28f;
constexpr float kSnowmanPickupDistance = 1.20f;
constexpr float kCoinScore = 10.0f;
constexpr int kFlipBonusScore = 100;
constexpr float kFlipBonusAngle = 4.36f;
constexpr float kScorePopupDuration = 0.85f;
constexpr float kMaxFuel = 100.0f;
constexpr float kFuelBurnPerSecond = 10.0f;
constexpr float kThrottleSpeed = 37.0f;
constexpr float kBrakeSpeed = -18.0f;
constexpr float kMotorTorque = 44.0f;
constexpr float kAirControlTorque = 11.5f;
constexpr float kHeadRadius = 0.22f;
constexpr b2Vec2 kDriverLocalPos = {-0.18f, -0.50f};
constexpr float kBodyVisualYOffset = -3.0f;

inline float clampf(float value, float lo, float hi)
{
    return std::max(lo, std::min(value, hi));
}

inline float length(b2Vec2 v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

} // namespace ridge_dash::game_config

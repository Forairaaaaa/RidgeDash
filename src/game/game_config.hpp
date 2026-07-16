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
constexpr float kGiantFleaGenerateAhead = 360.0f;
constexpr float kHelmetGenerateAhead = 360.0f;
constexpr float kMagnetGenerateAhead = 360.0f;
constexpr float kGroundThickness = 0.55f;
constexpr float kWheelRadius = 0.42f;
constexpr float kFuelCanRadius = 0.42f;
constexpr float kCoinRadius = 0.38f;
constexpr float kFleaRadius = 0.44f;
constexpr float kRocketRadius = 0.62f;
constexpr float kCactusRadius = 0.72f;
constexpr float kSnowmanRadius = 0.64f;
constexpr float kGiantFleaRadius = 0.56f;
constexpr float kHelmetRadius = 0.46f;
constexpr float kMagnetRadius = 0.48f;
constexpr float kFuelPickupDistance = 1.08f;
constexpr float kCoinPickupDistance = 0.96f;
constexpr float kFleaPickupDistance = 1.05f;
constexpr float kRocketPickupDistance = 1.16f;
constexpr float kCactusPickupDistance = 1.28f;
constexpr float kSnowmanPickupDistance = 1.20f;
constexpr float kGiantFleaPickupDistance = 1.14f;
constexpr float kHelmetPickupDistance = 1.06f;
constexpr float kMagnetPickupDistance = 1.08f;
constexpr float kMagnetDuration = 20.0f;
constexpr float kMagnetAttractRadius = 8.0f;
constexpr float kMagnetAttractSpeed = 14.0f;
constexpr float kCoinScore = 10.0f;

// Coin cluster layout: coins are placed as shapes (lines/waves/arcs/polygons),
// on the ground or floating in the air, with slope- and rocket-coupled odds.
constexpr float kCoinSpacing = 0.92f;       // spacing between coins in a shape (~diameter)
constexpr float kCoinClusterGapMin = 24.0f; // min x gap between shape clusters
constexpr float kCoinClusterGapMax = 48.0f; // max x gap between shape clusters
constexpr float kCoinGroundLift = 1.25f;    // ground clusters float this far above terrain
constexpr float kCoinMinClearance = 0.85f;  // no coin ever sits closer than this to the ground
constexpr float kCoinAirLiftMin = 3.4f;     // air clusters: min height above terrain
constexpr float kCoinAirLiftMax = 7.0f;     // air clusters: max height above terrain
constexpr float kCoinAirChance = 0.34f;     // base chance a cluster is placed in the air
constexpr float kCoinSlopeStep = 1.4f;      // dx used to sample local terrain slope
constexpr float kCoinSteepSlope = 0.55f;    // |slope| above this counts as a launch ramp
constexpr float kCoinRocketRadius = 9.0f;   // couple to a rocket within this x distance
constexpr float kCoinRocketAirBoost = 0.5f; // added air chance near a rocket

constexpr int kFlipBonusScore = 100;      // back flip score
constexpr int kFrontFlipBonusScore = 150; // front flip score (1.5x, harder to land)
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

// Touchdown sound thresholds (downward speed at impact, m/s).
constexpr float kLandingSoundMinSpeed = 2.0f; // below this, a touchdown is silent
constexpr float kHardLandingSpeed = 8.5f;     // above this, play the heavy landing sound

// BGM calm <-> intense switching (total speed, m/s), with hysteresis + dwell.
constexpr float kBgmIntenseSpeed = 9.0f; // above this -> switch to intense music
constexpr float kBgmCalmSpeed = 5.0f;    // sustained below this -> switch back to calm
constexpr float kBgmCalmDwell = 2.4f;    // seconds below calm speed before switching back

inline float clampf(float value, float lo, float hi)
{
    return std::max(lo, std::min(value, hi));
}

inline float length(b2Vec2 v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

} // namespace ridge_dash::game_config

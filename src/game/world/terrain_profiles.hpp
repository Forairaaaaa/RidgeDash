/**
 * @file terrain_profiles.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <array>
#include <cstddef>

namespace ridge_dash {

enum class TerrainBiome {
    Mountain,
    Stone,
    Desert,
    Snow,
};

struct TerrainProfile {
    const char* name = "";
    TerrainBiome biome = TerrainBiome::Mountain;
    float minLength = 0.0f;
    float maxLength = 0.0f;
    float rollingMin = 0.0f;
    float rollingMax = 0.0f;
    float climbMin = 0.0f;
    float climbMax = 0.0f;
    float dropMin = 0.0f;
    float dropMax = 0.0f;
    float climbWeight = 0.0f;
    float dropWeight = 0.0f;
    float slopeBlend = 0.0f;
    float noiseBase = 0.0f;
    float noiseGrowth = 0.0f;
    float minSlope = 0.0f;
    float maxSlope = 0.0f;
    float bumpChanceBase = 0.0f;
    float bumpChanceGrowth = 0.0f;
    float bumpUp = 0.0f;
    float bumpDown = 0.0f;
    float kickChanceBase = 0.0f;
    float kickChanceGrowth = 0.0f;
    float kick = 0.0f;
    float breakChance = 0.0f;
};

inline constexpr size_t kTerrainStoneProfileIndex = 4;
inline constexpr size_t kTerrainDesertProfileIndex = 5;
inline constexpr size_t kTerrainSnowProfileIndex = 6;

inline constexpr std::array<TerrainProfile, 7> kTerrainProfiles = {{
    {"rolling", TerrainBiome::Mountain,
     42.0f,     82.0f,
     -0.66f,    0.72f,
     -0.96f,    -0.48f,
     0.44f,     0.88f,
     0.30f,     0.32f,
     0.29f,     0.056f,
     0.064f,    -1.04f,
     1.08f,     0.044f,
     0.032f,    -1.18f,
     1.02f,     0.032f,
     0.030f,    0.62f,
     0.008f},
    {"ridges", TerrainBiome::Mountain,
     42.0f,    82.0f,
     -0.88f,   0.96f,
     -1.18f,   -0.62f,
     0.58f,    1.12f,
     0.32f,    0.36f,
     0.36f,    0.074f,
     0.094f,   -1.30f,
     1.34f,    0.070f,
     0.056f,   -1.55f,
     1.32f,    0.064f,
     0.046f,   0.88f,
     0.018f},
    {"steps", TerrainBiome::Mountain,
     34.0f,   68.0f,
     -0.62f,  0.70f,
     -1.30f,  -0.68f,
     0.64f,   1.24f,
     0.36f,   0.36f,
     0.40f,   0.064f,
     0.074f,  -1.38f,
     1.40f,   0.096f,
     0.064f,  -1.88f,
     1.64f,   0.044f,
     0.052f,  1.08f,
     0.026f},
    {"valley", TerrainBiome::Mountain,
     52.0f,    98.0f,
     -0.76f,   0.82f,
     -1.08f,   -0.52f,
     0.50f,    1.04f,
     0.40f,    0.28f,
     0.27f,    0.048f,
     0.062f,   -1.16f,
     1.20f,    0.048f,
     0.034f,   -1.34f,
     1.12f,    0.038f,
     0.032f,   0.72f,
     0.016f},
    {"stone", TerrainBiome::Stone,
     28.0f,   58.0f,
     -0.92f,  0.98f,
     -1.74f,  -0.92f,
     0.88f,   1.68f,
     0.42f,   0.43f,
     0.52f,   0.092f,
     0.126f,  -1.78f,
     1.82f,   0.135f,
     0.092f,  -2.32f,
     2.08f,   0.095f,
     0.074f,  1.42f,
     0.046f},
    {"desert", TerrainBiome::Desert,
     46.0f,    92.0f,
     -0.50f,   0.58f,
     -1.02f,   -0.42f,
     0.40f,    0.96f,
     0.26f,    0.30f,
     0.23f,    0.035f,
     0.050f,   -1.04f,
     1.08f,    0.035f,
     0.030f,   -0.92f,
     0.82f,    0.026f,
     0.026f,   0.52f,
     0.010f},
    {"snow", TerrainBiome::Snow,
     44.0f,  88.0f,
     -0.72f, 0.82f,
     -1.18f, -0.54f,
     0.54f,  1.22f,
     0.30f,  0.34f,
     0.30f,  0.046f,
     0.064f, -1.22f,
     1.26f,  0.060f,
     0.044f, -1.36f,
     1.18f,  0.040f,
     0.034f, 0.74f,
     0.012f},
}};

} // namespace ridge_dash

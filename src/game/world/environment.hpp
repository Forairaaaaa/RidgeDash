/**
 * @file environment.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "game/world/terrain_profiles.hpp"
#include "platform/raylib_compat.hpp"

#include <cstdint>

namespace ridge_dash {

class Environment {
public:
    void loadAssets();
    void unloadAssets();
    void reset(uint32_t seed);
    void updateBiome(TerrainBiome targetBiome, float dt);
    void drawBackground(Vector2 camera, float distance) const;
    float dayAmount(float distance) const;

private:
    Texture2D _sun{};
    Texture2D _sunBlink{};
    Texture2D _moon{};
    Texture2D _moonBlink{};
    Texture2D _sailboat{};
    TerrainBiome _fromBiome = TerrainBiome::Mountain;
    TerrainBiome _targetBiome = TerrainBiome::Mountain;
    float _biomeBlend = 1.0f;
    float _time = 0.0f;
    uint32_t _seed = 0;
};

} // namespace ridge_dash

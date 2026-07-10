/**
 * @file terrain.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "game/world/terrain_profiles.hpp"
#include "platform/raylib_compat.hpp"

#include <box2d/box2d.h>
#include <cstddef>
#include <random>
#include <vector>

namespace ridge_dash {

struct TerrainSample {
    float x = 0.0f;
    float y = 0.0f;
    TerrainBiome biome = TerrainBiome::Mountain;
};

class TerrainSystem {
public:
    void reset(float startX, b2WorldId worldId, std::mt19937& rng);
    void clearBodies();
    void appendUntil(float targetX, std::mt19937& rng);
    void trimBehind(float minX);

    float heightAt(float x) const;
    TerrainBiome biomeAt(float x) const;
    TerrainSample sampleAt(float x, float lookAhead, std::mt19937& rng);
    bool isTerrainShape(b2ShapeId shapeId) const;
    void draw(Vector2 camera) const;

private:
    struct Point {
        float x = 0.0f;
        float y = 0.0f;
        size_t profileIndex = 0;
    };

    struct Body {
        b2BodyId bodyId = b2_nullBodyId;
        float maxX = 0.0f;
    };

    void clear();
    void selectProfile(float x, std::mt19937& rng);
    b2BodyId createBody(const Point& a, const Point& b);

    b2WorldId _worldId = b2_nullWorldId;
    std::vector<Point> _points;
    std::vector<Body> _bodies;
    float _generatedUntil = 0.0f;
    float _currentY = 10.4f;
    float _slope = 0.0f;
    float _targetSlope = 0.0f;
    float _profileEndX = 0.0f;
    int _targetSteps = 0;
    size_t _profileIndex = 0;
};

} // namespace ridge_dash

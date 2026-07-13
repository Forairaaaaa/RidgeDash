/**
 * @file terrain.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/world/terrain.hpp"

#include "game/game_config.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <random>
#include <string>

namespace ridge_dash {
namespace {
using namespace game_config;

const TerrainProfile& profileAt(size_t index)
{
    return kTerrainProfiles[index % kTerrainProfiles.size()];
}

float frictionForBiome(TerrainBiome biome)
{
    switch (biome) {
        case TerrainBiome::Stone:
            return 0.86f;
        case TerrainBiome::Desert:
            return 1.08f;
        case TerrainBiome::Snow:
            return 0.62f;
        case TerrainBiome::Mountain:
        default:
            return 0.96f;
    }
}

std::string lowerCopy(const char* value)
{
    std::string result = value ? value : "";
    for (char& ch : result) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return result;
}

int forcedTerrainProfileIndex()
{
    const char* value = std::getenv("RIDGEDASH_TEST_TERRAIN");
    if (!value || value[0] == '\0') {
        value = std::getenv("RIDGEDASH_TEST_BIOME");
    }
    const std::string terrain = lowerCopy(value);
    if (terrain.empty() || terrain == "off" || terrain == "random") {
        return -1;
    }
    if (terrain == "stone" || terrain == "rock") {
        return static_cast<int>(kTerrainStoneProfileIndex);
    }
    if (terrain == "desert" || terrain == "sand") {
        return static_cast<int>(kTerrainDesertProfileIndex);
    }
    if (terrain == "snow" || terrain == "ice") {
        return static_cast<int>(kTerrainSnowProfileIndex);
    }
    if (terrain == "mountain") {
        return 0;
    }
    for (size_t i = 0; i < kTerrainProfiles.size(); ++i) {
        if (terrain == kTerrainProfiles[i].name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Vector2 worldToScreen(Vector2 camera, b2Vec2 p)
{
    return {p.x * kPixelsPerMeter - camera.x, p.y * kPixelsPerMeter - camera.y};
}

} // namespace

void TerrainSystem::reset(float startX, b2WorldId worldId, std::mt19937& rng)
{
    if (b2World_IsValid(_worldId)) {
        for (const Body& body : _bodies) {
            if (b2Body_IsValid(body.bodyId)) {
                b2DestroyBody(body.bodyId);
            }
        }
    }

    clear();
    _worldId = worldId;
    _points.reserve(static_cast<size_t>(kTerrainGenerateAhead / kTerrainStep) + 16);

    _currentY = 10.4f;
    _slope = 0.0f;
    _targetSlope = 0.0f;
    _targetSteps = 0;
    _profileIndex = 0;
    _profileEndX = kTerrainFlatUntilX;
    _generatedUntil = kTerrainStartX;
    _points.push_back({_generatedUntil, _currentY, _profileIndex});

    appendUntil(startX + kTerrainGenerateAhead, rng);
}

void TerrainSystem::clear()
{
    _points.clear();
    _bodies.clear();
    _generatedUntil = 0.0f;
    _currentY = 10.4f;
    _slope = 0.0f;
    _targetSlope = 0.0f;
    _profileEndX = 0.0f;
    _targetSteps = 0;
    _profileIndex = 0;
}

void TerrainSystem::clearBodies()
{
    _bodies.clear();
    _worldId = b2_nullWorldId;
}

void TerrainSystem::selectProfile(float x, std::mt19937& rng)
{
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    std::uniform_real_distribution<float> lengthJitter(0.0f, 1.0f);

    const int forcedProfileIndex = forcedTerrainProfileIndex();
    if (forcedProfileIndex >= 0) {
        _profileIndex = static_cast<size_t>(forcedProfileIndex);
    } else if (x < 48.0f) {
        _profileIndex = 0;
    } else {
        const float difficulty = clampf((x - 44.0f) / 460.0f, 0.0f, 1.0f);
        const float stoneChance = x < 120.0f ? 0.0f : 0.08f + 0.16f * difficulty;
        const float desertChance = x < 90.0f ? 0.0f : 0.10f + 0.10f * difficulty;
        const float snowChance = x < 150.0f ? 0.0f : 0.08f + 0.10f * difficulty;
        const float roll = chance(rng);
        if (roll < stoneChance) {
            _profileIndex = kTerrainStoneProfileIndex;
        } else if (roll < stoneChance + desertChance) {
            _profileIndex = kTerrainDesertProfileIndex;
        } else if (roll < stoneChance + desertChance + snowChance) {
            _profileIndex = kTerrainSnowProfileIndex;
        } else {
            const float mountainRoll = (roll - stoneChance - desertChance - snowChance) /
                                       std::max(0.001f, 1.0f - stoneChance - desertChance - snowChance);
            if (mountainRoll < 0.22f - 0.08f * difficulty) {
                _profileIndex = 0;
            } else if (mountainRoll < 0.54f) {
                _profileIndex = 1;
            } else if (mountainRoll < 0.82f + 0.08f * difficulty) {
                _profileIndex = 2;
            } else {
                _profileIndex = 3;
            }
        }
    }

    const TerrainProfile& profile = profileAt(_profileIndex);
    _profileEndX = x + profile.minLength + (profile.maxLength - profile.minLength) * lengthJitter(rng);
    _targetSteps = 0;
}

void TerrainSystem::appendUntil(float targetX, std::mt19937& rng)
{
    std::uniform_real_distribution<float> noise(-1.0f, 1.0f);
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);

    while (_generatedUntil + kTerrainStep <= targetX) {
        const Point prev = _points.back();
        const float x = _generatedUntil + kTerrainStep;

        if (x < kTerrainFlatUntilX) {
            _currentY = 10.4f;
            _slope = 0.0f;
        } else {
            if (x >= _profileEndX) {
                selectProfile(x, rng);
            }

            const TerrainProfile& profile = profileAt(_profileIndex);
            if (_targetSteps <= 0) {
                const float roll = chance(rng);
                if (roll < profile.climbWeight) {
                    std::uniform_real_distribution<float> slopeDist(profile.climbMin, profile.climbMax);
                    _targetSlope = slopeDist(rng);
                } else if (roll < profile.climbWeight + profile.dropWeight) {
                    std::uniform_real_distribution<float> slopeDist(profile.dropMin, profile.dropMax);
                    _targetSlope = slopeDist(rng);
                } else {
                    std::uniform_real_distribution<float> slopeDist(profile.rollingMin, profile.rollingMax);
                    _targetSlope = slopeDist(rng);
                }
                std::uniform_int_distribution<int> stepDist(3, 8);
                _targetSteps = stepDist(rng);
            }

            const float difficulty = clampf((x - 28.0f) / 220.0f, 0.0f, 1.0f);
            _slope += (_targetSlope - _slope) * profile.slopeBlend +
                      noise(rng) * (profile.noiseBase + profile.noiseGrowth * difficulty);
            _slope = clampf(_slope, profile.minSlope, profile.maxSlope);
            _currentY += _slope;

            if (chance(rng) < profile.bumpChanceBase + profile.bumpChanceGrowth * difficulty) {
                _currentY += chance(rng) < 0.56f ? profile.bumpUp : profile.bumpDown;
            }

            if (chance(rng) < profile.kickChanceBase + profile.kickChanceGrowth * difficulty) {
                _slope += chance(rng) < 0.5f ? -profile.kick : profile.kick;
            }

            if (chance(rng) < profile.breakChance * difficulty) {
                _currentY += chance(rng) < 0.5f ? -1.86f : 1.58f;
                _slope *= -0.42f;
            }

            _currentY = clampf(_currentY, 4.8f, 13.7f);
            --_targetSteps;
        }

        _points.push_back({x, _currentY, _profileIndex});
        _generatedUntil = x;

        if (b2World_IsValid(_worldId)) {
            b2BodyId bodyId = createBody(prev, _points.back());
            if (b2Body_IsValid(bodyId)) {
                _bodies.push_back({bodyId, std::max(prev.x, x)});
            }
        }
    }
}

float TerrainSystem::heightAt(float x) const
{
    if (_points.empty()) {
        return 10.4f;
    }
    if (x <= _points.front().x) {
        return _points.front().y;
    }
    if (x >= _points.back().x) {
        return _points.back().y;
    }

    const auto it = std::lower_bound(_points.begin(), _points.end(), x, [](const Point& point, float value) {
        return point.x < value;
    });
    if (it == _points.begin()) {
        return it->y;
    }

    const Point& a = *(it - 1);
    const Point& b = *it;
    const float t = (x - a.x) / (b.x - a.x);
    return a.y + (b.y - a.y) * t;
}

TerrainBiome TerrainSystem::biomeAt(float x) const
{
    if (_points.empty()) {
        return TerrainBiome::Mountain;
    }
    if (x <= _points.front().x) {
        return profileAt(_points.front().profileIndex).biome;
    }

    const auto it = std::lower_bound(_points.begin(), _points.end(), x, [](const Point& point, float value) {
        return point.x < value;
    });
    if (it == _points.begin()) {
        return profileAt(it->profileIndex).biome;
    }
    return profileAt((it - 1)->profileIndex).biome;
}

TerrainSample TerrainSystem::sampleAt(float x, float lookAhead, std::mt19937& rng)
{
    appendUntil(x + lookAhead, rng);
    if (_points.empty()) {
        return TerrainSample{x, 10.4f, TerrainBiome::Mountain};
    }
    if (x <= _points.front().x) {
        return TerrainSample{x, _points.front().y, profileAt(_points.front().profileIndex).biome};
    }
    if (x >= _points.back().x) {
        return TerrainSample{x, _points.back().y, profileAt(_points.back().profileIndex).biome};
    }

    const auto it = std::lower_bound(_points.begin(), _points.end(), x, [](const Point& point, float value) {
        return point.x < value;
    });
    if (it == _points.begin()) {
        return TerrainSample{x, it->y, profileAt(it->profileIndex).biome};
    }

    const Point& a = *(it - 1);
    const Point& b = *it;
    const float t = (x - a.x) / (b.x - a.x);
    return TerrainSample{x, a.y + (b.y - a.y) * t, profileAt(a.profileIndex).biome};
}

b2BodyId TerrainSystem::createBody(const Point& a, const Point& b)
{
    if (!b2World_IsValid(_worldId)) {
        return b2_nullBodyId;
    }

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.material.friction = frictionForBiome(profileAt(a.profileIndex).biome);
    shapeDef.material.restitution = 0.02f;
    shapeDef.enableSensorEvents = true;

    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float len = std::max(0.1f, std::sqrt(dx * dx + dy * dy));
    const float angle = std::atan2(dy, dx);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f + kGroundThickness * 0.5f};
    bodyDef.rotation = b2MakeRot(angle);
    b2BodyId bodyId = b2CreateBody(_worldId, &bodyDef);
    b2Polygon box = b2MakeBox(len * 0.5f + 0.04f, kGroundThickness * 0.5f);
    b2CreatePolygonShape(bodyId, &shapeDef, &box);
    return bodyId;
}

void TerrainSystem::trimBehind(float minX)
{
    if (b2World_IsValid(_worldId)) {
        auto bodyIt = _bodies.begin();
        while (bodyIt != _bodies.end()) {
            if (bodyIt->maxX >= minX) {
                ++bodyIt;
                continue;
            }
            if (b2Body_IsValid(bodyIt->bodyId)) {
                b2DestroyBody(bodyIt->bodyId);
            }
            bodyIt = _bodies.erase(bodyIt);
        }
    }

    while (_points.size() > 2 && _points[1].x < minX) {
        _points.erase(_points.begin());
    }
}

bool TerrainSystem::isTerrainShape(b2ShapeId shapeId) const
{
    if (!b2Shape_IsValid(shapeId)) {
        return false;
    }
    const b2BodyId bodyId = b2Shape_GetBody(shapeId);
    for (const Body& terrain : _bodies) {
        if (b2Body_IsValid(terrain.bodyId) && B2_ID_EQUALS(terrain.bodyId, bodyId)) {
            return true;
        }
    }
    return false;
}

void TerrainSystem::draw(Vector2 camera) const
{
    if (_points.size() < 2) {
        return;
    }

    for (size_t i = 1; i < _points.size(); ++i) {
        const TerrainProfile& profile = profileAt(_points[i].profileIndex);
        const bool stone = profile.biome == TerrainBiome::Stone;
        const bool desert = profile.biome == TerrainBiome::Desert;
        const bool snow = profile.biome == TerrainBiome::Snow;
        const Vector2 a = worldToScreen(camera, {_points[i - 1].x, _points[i - 1].y});
        const Vector2 b = worldToScreen(camera, {_points[i].x, _points[i].y});
        if ((a.x < -30.0f && b.x < -30.0f) || (a.x > kScreenWidth + 30.0f && b.x > kScreenWidth + 30.0f)) {
            continue;
        }
        const Color soil = stone    ? Color{48, 49, 54, 255}
                           : desert ? Color{91, 62, 44, 255}
                           : snow   ? Color{45, 63, 78, 255}
                                    : Color{56, 43, 38, 255};
        const Color soilLight = stone    ? Color{68, 69, 76, 255}
                                : desert ? Color{115, 80, 55, 255}
                                : snow   ? Color{64, 84, 102, 255}
                                         : Color{75, 59, 48, 255};
        const Color soilDark  = stone    ? Color{32, 33, 36, 255}
                                : desert ? Color{68, 44, 30, 255}
                                : snow   ? Color{30, 44, 56, 255}
                                         : Color{38, 30, 26, 255};
        const Color subTop = stone    ? Color{76, 80, 87, 255}
                             : desert ? Color{166, 115, 58, 255}
                             : snow   ? Color{125, 165, 184, 255}
                                      : Color{55, 78, 51, 255};
        const Color top = stone    ? Color{140, 147, 153, 255}
                          : desert ? Color{225, 180, 88, 255}
                          : snow   ? Color{232, 248, 255, 255}
                                   : Color{128, 188, 89, 255};
        const Color marker = stone    ? Color{96, 101, 108, 255}
                             : desert ? Color{130, 83, 49, 255}
                             : snow   ? Color{151, 191, 210, 255}
                                      : Color{86, 62, 45, 255};
        DrawLineEx({std::round(a.x), std::round(a.y + 9.0f)}, {std::round(b.x), std::round(b.y + 9.0f)}, 18.0f, soil);
        DrawLineEx({std::round(a.x), std::round(a.y + 3.0f)}, {std::round(b.x), std::round(b.y + 3.0f)}, 6.0f, subTop);
        DrawLineEx({std::round(a.x), std::round(a.y)},
                   {std::round(b.x), std::round(b.y)},
                   stone ? 4.0f : (desert ? 3.5f : (snow ? 4.0f : 3.0f)),
                   top);

        // Base fill from surface level down to screen bottom.
        // A single solid pass guarantees zero gaps, even on steep slopes.
        {
            const float screenH = static_cast<float>(kScreenHeight);
            const Vector2 sa = {a.x, a.y + 9.0f};
            const Vector2 sb = {b.x, b.y + 9.0f};
            const Vector2 bl = {a.x, screenH};
            const Vector2 br = {b.x, screenH};
            DrawTriangle(sa, br, sb, soilDark);
            DrawTriangle(sa, bl, br, soilDark);
        }

        // Two lighter layers overlaid — any micro-gap just shows the
        // similar-coloured base fill, reading as natural noise.
        {
            const float screenH = static_cast<float>(kScreenHeight);
            const float ax = _points[i - 1].x;
            const float bx = _points[i].x;

            const float n0a = std::sin(ax * 0.7f) * 2.0f + std::cos(ax * 1.4f) * 1.5f;
            const float n0b = std::sin(bx * 0.7f) * 2.0f + std::cos(bx * 1.4f) * 1.5f;
            const float n1a = std::sin(ax * 0.5f + 1.8f) * 2.5f + std::cos(ax * 1.1f) * 1.0f;
            const float n1b = std::sin(bx * 0.5f + 1.8f) * 2.5f + std::cos(bx * 1.1f) * 1.0f;

            const float offA[2] = {16.0f + n0a, 34.0f + n1a};
            const float offB[2] = {16.0f + n0b, 34.0f + n1b};
            const Color colors[2] = {soilLight, soil};
            for (int layer = 0; layer < 2; ++layer) {
                const Vector2 ta = {a.x, a.y + offA[layer]};
                const Vector2 tb = {b.x, b.y + offB[layer]};
                const Vector2 bl = {a.x, screenH};
                const Vector2 br = {b.x, screenH};
                DrawTriangle(ta, br, tb, colors[layer]);
                DrawTriangle(ta, bl, br, colors[layer]);
            }
        }

        // Scattered soil markers — small rocks/pebbles at varying depths.
        // Pseudo-random placement based on world position so markers stay
        // fixed as the camera scrolls, without a per-frame RNG call.
        {
            const float wx = _points[i - 1].x;
            auto hash = [](float seed) -> float {
                float v = std::sin(seed * 127.1f) * 43758.5453f;
                return v - std::floor(v);
            };
            const float r0 = hash(wx);
            const float r1 = hash(wx + 5.0f);
            const float r2 = hash(wx + 9.0f);

            // Larger pebbles: ~30% chance per point, scattered through the soil
            if (r0 < 0.30f) {
                const float depth = 12.0f + r1 * 48.0f;
                const int size = r2 < 0.25f ? 2 : (r2 < 0.70f ? 3 : 4);
                const Color c = depth < 28.0f ? marker : soilDark;
                DrawRectangle(static_cast<int>(std::round(a.x)),
                              static_cast<int>(std::round(a.y + depth)), size, size, c);
            }

            // Smaller speckles at higher density for texture
            if (r1 < 0.28f) {
                const float depth = 10.0f + r2 * 50.0f;
                DrawRectangle(static_cast<int>(std::round(a.x)),
                              static_cast<int>(std::round(a.y + depth)), 2, 2, marker);
            }
        }
    }
}

} // namespace ridge_dash

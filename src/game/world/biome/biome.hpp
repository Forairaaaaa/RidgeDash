/**
 * @file biome.hpp
 * @author Forairaaaaa
 * @brief Per-biome terrain surface + background rendering. Each concrete biome
 *        is implemented in its own biome_*.cpp translation unit.
 * @version 0.1
 * @date 2026-07-14
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "game/world/terrain_profiles.hpp"
#include "game/game_config.hpp"
#include "platform/raylib_compat.hpp"

#include <cmath>
#include <cstdint>

namespace ridge_dash {

// ── Terrain surface colour palette for one biome ──────────────────────────
struct TerrainColors {
    Color soil;
    Color soilLight;
    Color soilDark;
    Color subTop;
    Color top;
    Color marker;
};

// ── Shared biome rendering helpers ──────────────────────────────────────
namespace biome_detail {

inline uint32_t hash32(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

inline float random01(uint32_t seed, int index)
{
    return static_cast<float>(hash32(seed + static_cast<uint32_t>(index) * 747796405U) & 0xffffU) / 65535.0f;
}

inline int wrapToRange(int value, int range)
{
    const int wrapped = value % range;
    return wrapped < 0 ? wrapped + range : wrapped;
}

inline Color mixColor(Color a, Color b, float t)
{
    t = game_config::clampf(t, 0.0f, 1.0f);
    return Color{static_cast<unsigned char>(a.r + (b.r - a.r) * t),
                 static_cast<unsigned char>(a.g + (b.g - a.g) * t),
                 static_cast<unsigned char>(a.b + (b.b - a.b) * t),
                 static_cast<unsigned char>(a.a + (b.a - a.a) * t)};
}

inline Color withAlpha(Color color, float alpha)
{
    color.a = static_cast<unsigned char>(static_cast<float>(color.a) * game_config::clampf(alpha, 0.0f, 1.0f));
    return color;
}

constexpr float kPi = 3.14159265358979323846f;

// Generic "looping slot" particle renderer: no state, purely seed+time computed positions.
// Particles cycle from top to bottom via fmod — no allocation/deallocation overhead.
inline void drawLoopingParticles(uint32_t seed,
                                 float time,
                                 Vector2 camera,
                                 float parallax,
                                 float fallSpeed,
                                 float swayAmp,
                                 float swayFreq,
                                 int count,
                                 int pixelSize,
                                 Color color,
                                 float alpha)
{
    using namespace game_config;

    if (alpha <= 0.01f) {
        return;
    }

    const float scrollX = camera.x * parallax;
    constexpr float kSpawnHeight = kScreenHeight + 20.0f;

    for (int i = 0; i < count; ++i) {
        const uint32_t particleSeed = hash32(seed + static_cast<uint32_t>(i) * 0x9E3779B9U);
        const float baseWorldX = static_cast<float>(particleSeed % 400U) - 40.0f;
        const float phase = random01(seed, i + 500) * kPi * 2.0f;
        const float fallOffset = random01(seed, i + 700) * kSpawnHeight;

        const float y = std::fmod(time * fallSpeed + fallOffset, kSpawnHeight) - 10.0f;
        const float sway = std::sin(time * swayFreq + phase) * swayAmp;
        const float worldX = baseWorldX + sway;

        const int x = wrapToRange(static_cast<int>(std::round(worldX - scrollX)), kScreenWidth + 80) - 40;
        const int yi = static_cast<int>(y);
        if (yi < -4 || yi > kScreenHeight + 4) {
            continue;
        }

        const int size = pixelSize + static_cast<int>(particleSeed % 2U);
        DrawRectangle(x, yi, size, size, withAlpha(color, alpha));
    }
}

} // namespace biome_detail

// ── Abstract biome renderer ──────────────────────────────────────────────
class IBiomeRenderer {
public:
    virtual ~IBiomeRenderer() = default;
    virtual float friction() const = 0;
    virtual const TerrainColors& terrainColors() const = 0;
    virtual float surfaceThickness() const = 0;
    virtual void drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const = 0;
    virtual void drawHorizon(Vector2 camera, float day, float alpha) const = 0;
    virtual void drawAmbientParticles(Vector2 camera, float day, float alpha, uint32_t seed, float time) const {}
};

// Concrete renderers — one per biome, implemented in biome_<name>.cpp.
class MountainRenderer : public IBiomeRenderer {
public:
    float friction() const override;
    const TerrainColors& terrainColors() const override;
    float surfaceThickness() const override;
    void drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const override;
    void drawHorizon(Vector2 camera, float day, float alpha) const override;
    void drawAmbientParticles(Vector2 camera, float day, float alpha, uint32_t seed, float time) const override;
};

class StoneRenderer : public IBiomeRenderer {
public:
    float friction() const override;
    const TerrainColors& terrainColors() const override;
    float surfaceThickness() const override;
    void drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const override;
    void drawHorizon(Vector2 camera, float day, float alpha) const override;
    void drawAmbientParticles(Vector2 camera, float day, float alpha, uint32_t seed, float time) const override;
};

class DesertRenderer : public IBiomeRenderer {
public:
    float friction() const override;
    const TerrainColors& terrainColors() const override;
    float surfaceThickness() const override;
    void drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const override;
    void drawHorizon(Vector2 camera, float day, float alpha) const override;
    void drawAmbientParticles(Vector2 camera, float day, float alpha, uint32_t seed, float time) const override;
};

class SnowRenderer : public IBiomeRenderer {
public:
    float friction() const override;
    const TerrainColors& terrainColors() const override;
    float surfaceThickness() const override;
    void drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const override;
    void drawHorizon(Vector2 camera, float day, float alpha) const override;
    void drawAmbientParticles(Vector2 camera, float day, float alpha, uint32_t seed, float time) const override;
};

// Factory.
const IBiomeRenderer& biomeRendererFor(TerrainBiome biome);

} // namespace ridge_dash

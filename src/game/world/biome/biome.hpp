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
#include "platform/raylib_compat.hpp"

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

// ── Abstract biome renderer ──────────────────────────────────────────────
class IBiomeRenderer {
public:
    virtual ~IBiomeRenderer() = default;
    virtual float friction() const = 0;
    virtual const TerrainColors& terrainColors() const = 0;
    virtual float surfaceThickness() const = 0;
    virtual void drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const = 0;
    virtual void drawHorizon(Vector2 camera, float day, float alpha) const = 0;
};

// Concrete renderers — one per biome, implemented in biome_<name>.cpp.
class MountainRenderer : public IBiomeRenderer {
public:
    float friction() const override;
    const TerrainColors& terrainColors() const override;
    float surfaceThickness() const override;
    void drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const override;
    void drawHorizon(Vector2 camera, float day, float alpha) const override;
};

class StoneRenderer : public IBiomeRenderer {
public:
    float friction() const override;
    const TerrainColors& terrainColors() const override;
    float surfaceThickness() const override;
    void drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const override;
    void drawHorizon(Vector2 camera, float day, float alpha) const override;
};

class DesertRenderer : public IBiomeRenderer {
public:
    float friction() const override;
    const TerrainColors& terrainColors() const override;
    float surfaceThickness() const override;
    void drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const override;
    void drawHorizon(Vector2 camera, float day, float alpha) const override;
};

class SnowRenderer : public IBiomeRenderer {
public:
    float friction() const override;
    const TerrainColors& terrainColors() const override;
    float surfaceThickness() const override;
    void drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const override;
    void drawHorizon(Vector2 camera, float day, float alpha) const override;
};

// Factory.
const IBiomeRenderer& biomeRendererFor(TerrainBiome biome);

} // namespace ridge_dash

/**
 * @file stone_biome.cpp
 * @author Forairaaaaa
 * @brief Stone biome — rocky grey terrain, crag horizon.
 * @version 0.1
 * @date 2026-07-14
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/world/biome/biome.hpp"

#include "game/game_config.hpp"
#include "game/render_helpers.hpp"

#include <algorithm>
#include <cmath>

namespace ridge_dash {
namespace {

using namespace game_config;

uint32_t hash32(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

Color mixColor(Color a, Color b, float t)
{
    t = clampf(t, 0.0f, 1.0f);
    return Color{static_cast<unsigned char>(a.r + (b.r - a.r) * t),
                 static_cast<unsigned char>(a.g + (b.g - a.g) * t),
                 static_cast<unsigned char>(a.b + (b.b - a.b) * t),
                 static_cast<unsigned char>(a.a + (b.a - a.a) * t)};
}

Color withAlpha(Color color, float alpha)
{
    color.a = static_cast<unsigned char>(static_cast<float>(color.a) * clampf(alpha, 0.0f, 1.0f));
    return color;
}

// Stone uses the snow-strip shape but recoloured for rock.
void drawDistantSnowStrip(Vector2 camera,
                          float parallax,
                          int baseY,
                          Color mainColor,
                          Color capColor,
                          Color shadeColor,
                          float capCoverageScale = 1.0f)
{
    const float scroll = camera.x * parallax;
    const int step = 38;
    int start = -step - static_cast<int>(std::fmod(scroll, static_cast<float>(step)));
    if (start > 0) {
        start -= step;
    }
    const int valleyFloor = baseY - 6;

    for (int x = start; x < kScreenWidth + step; x += step) {
        const uint32_t ridgeSeed = hash32(static_cast<uint32_t>(x + static_cast<int>(scroll)) * 173U);
        const int width = 40 + static_cast<int>(ridgeSeed % 24U);
        const int left = x - width / 3;
        const int right = left + width;
        const int mid = left + static_cast<int>(width * (0.32f + static_cast<float>((ridgeSeed >> 8U) % 37U) / 100.0f));
        const int peak = baseY - 14 - static_cast<int>((ridgeSeed >> 16U) % 14U);

        for (int px = left; px < right; px += 2) {
            const float t = px < mid ? static_cast<float>(px - left) / static_cast<float>(std::max(1, mid - left))
                                     : static_cast<float>(right - px) / static_cast<float>(std::max(1, right - mid));
            const float ct = clampf(t, 0.0f, 1.0f);
            const float eased = ct * ct * (3.0f - 2.0f * ct);
            const int y = valleyFloor - static_cast<int>((valleyFloor - peak) * eased);
            DrawRectangle(px, y, 2, baseY - y, mainColor);
            if (px >= mid && y < baseY - 8) {
                DrawRectangle(px, y + 5, 2, std::max(2, (baseY - y) / 3), shadeColor);
            }
            if (capCoverageScale > 0.01f) {
                const int localHeight = baseY - y;
                if (localHeight > 6) {
                    const int worldPx = px + static_cast<int>(scroll);
                    const uint32_t capSeed = hash32(static_cast<uint32_t>(worldPx) * 233U + ridgeSeed);
                    const float baseCoverage = 0.38f + static_cast<float>((capSeed >> 8U) & 0xffU) / 255.0f * 0.14f;
                    const float coverage = baseCoverage * capCoverageScale;
                    int capHeight = std::max(1, static_cast<int>(localHeight * coverage));
                    for (int row = 0; row < capHeight; ++row) {
                        const int yy = y + row;
                        if (capCoverageScale > 0.6f && row == capHeight - 1 && (((worldPx / 2 + yy) % 2) == 0)) {
                            continue;
                        }
                        DrawRectangle(px, yy, 2, 1, capColor);
                    }
                }
            }
        }
    }
}

} // namespace

float StoneRenderer::friction() const
{
    return 0.86f;
}

const TerrainColors& StoneRenderer::terrainColors() const
{
    static const TerrainColors c{Color{48, 49, 54, 255},
                                 Color{68, 69, 76, 255},
                                 Color{32, 33, 36, 255},
                                 Color{76, 80, 87, 255},
                                 Color{140, 147, 153, 255},
                                 Color{96, 101, 108, 255}};
    return c;
}

float StoneRenderer::surfaceThickness() const
{
    return 4.0f;
}

void StoneRenderer::drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const
{
    auto hash = [](float seed) -> float {
        float v = std::sin(seed * 127.1f) * 43758.5453f;
        return v - std::floor(v);
    };
    const float r0 = hash(worldX);
    const float r1 = hash(worldX + 5.0f);
    const float r2 = hash(worldX + 9.0f);

    if (r0 < 0.30f) {
        const float depth = 12.0f + r1 * 48.0f;
        const int size = r2 < 0.25f ? 2 : (r2 < 0.70f ? 3 : 4);
        DrawRectangle(static_cast<int>(std::round(a.x)),
                      static_cast<int>(std::round(a.y + depth)),
                      size,
                      size,
                      depth < 28.0f ? c.marker : c.soilDark);
    }
    if (r1 < 0.28f) {
        const float depth = 10.0f + r2 * 50.0f;
        DrawRectangle(static_cast<int>(std::round(a.x)), static_cast<int>(std::round(a.y + depth)), 2, 2, c.marker);
    }
}

void StoneRenderer::drawHorizon(Vector2 camera, float day, float alpha) const
{
    if (alpha <= 0.01f) {
        return;
    }
    const Color unused{0, 0, 0, 0};
    drawDistantSnowStrip(camera,
                         0.08f,
                         103,
                         withAlpha(mixColor(Color{34, 39, 45, 255}, Color{93, 102, 108, 255}, day), alpha),
                         withAlpha(mixColor(Color{104, 111, 117, 255}, Color{174, 182, 187, 255}, day), alpha * 0.85f),
                         withAlpha(mixColor(Color{30, 34, 40, 255}, Color{80, 87, 94, 255}, day), alpha * 0.55f),
                         0.22f);
    drawDistantSnowStrip(camera,
                         0.15f,
                         124,
                         withAlpha(mixColor(Color{41, 45, 50, 255}, Color{111, 118, 123, 255}, day), alpha),
                         withAlpha(mixColor(Color{119, 125, 130, 255}, Color{192, 198, 202, 255}, day), alpha * 0.85f),
                         withAlpha(mixColor(Color{36, 40, 45, 255}, Color{93, 99, 104, 255}, day), alpha * 0.55f),
                         0.22f);
}

} // namespace ridge_dash

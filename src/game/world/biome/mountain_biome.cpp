/**
 * @file mountain_biome.cpp
 * @author Forairaaaaa
 * @brief Mountain biome — green hills, soil/grass terrain, forested horizon.
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

void drawDistantDuneStrip(Vector2 camera,
                          float parallax,
                          int baseY,
                          Color mainColor,
                          Color topColor,
                          Color shadowColor,
                          float heightScale = 1.0f)
{
    const float scroll = camera.x * parallax;
    const int step = 46;
    int start = -step - static_cast<int>(std::fmod(scroll, static_cast<float>(step)));
    if (start > 0) {
        start -= step;
    }

    DrawRectangle(0, baseY - 3, kScreenWidth, 7, shadowColor);

    for (int x = start; x < kScreenWidth + step; x += step) {
        const uint32_t duneSeed = hash32(static_cast<uint32_t>(x + static_cast<int>(scroll)) * 97U);
        const int width = 58 + static_cast<int>(duneSeed % 40U);
        const int left = x - width / 4;
        const int right = left + width;
        const int crest = left + static_cast<int>(width * (0.2f + static_cast<float>((duneSeed >> 8U) % 61U) / 100.0f));
        const int height = static_cast<int>((10 + static_cast<int>((duneSeed >> 16U) % 16U)) * heightScale);

        for (int px = left; px < right; px += 4) {
            const float t = static_cast<float>(px - left) / static_cast<float>(right - left);
            const float ridge =
                1.0f - std::abs(static_cast<float>(px - crest) / static_cast<float>(right - left) * 2.0f);
            const float wave =
                std::sin((t + static_cast<float>((duneSeed >> 16U) & 0xffU) / 255.0f) * 3.14159f) * 0.22f;
            const int y = baseY - static_cast<int>(height * clampf(ridge + wave, 0.0f, 1.0f));
            DrawRectangle(px, y, 4, baseY + 2 - y, mainColor);
            if (px <= crest + 4 && y < baseY - 4) {
                DrawRectangle(px, y + 2, 4, 1, topColor);
            }
            if (px > crest + 4 && y < baseY - 5) {
                const float distT = clampf(
                    static_cast<float>(px - crest - 4) / static_cast<float>(std::max(1, right - crest)), 0.0f, 1.0f);
                const Color gradShadow = withAlpha(shadowColor, 0.35f + distT * 0.65f);
                DrawRectangle(px, y + 5, 4, std::max(1, baseY + 2 - (y + 5)), gradShadow);
            }
            if (((px + static_cast<int>(duneSeed)) % 19) < 4 && y < baseY - 8) {
                DrawRectangle(px, y + 6, 3, 1, topColor);
            }
        }
    }
}

} // namespace

float MountainRenderer::friction() const
{
    return 0.96f;
}

const TerrainColors& MountainRenderer::terrainColors() const
{
    static const TerrainColors c{Color{56, 43, 38, 255},
                                 Color{75, 59, 48, 255},
                                 Color{38, 30, 26, 255},
                                 Color{55, 78, 51, 255},
                                 Color{128, 188, 89, 255},
                                 Color{86, 62, 45, 255}};
    return c;
}

float MountainRenderer::surfaceThickness() const
{
    return 3.0f;
}

void MountainRenderer::drawMarkers(Vector2 a, float worldX, const TerrainColors& c) const
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
        DrawRectangle(
            static_cast<int>(std::round(a.x)), static_cast<int>(std::round(a.y + depth)), size, size, c.marker);
    }
    if (r1 < 0.28f) {
        const float depth = 10.0f + r2 * 50.0f;
        DrawRectangle(static_cast<int>(std::round(a.x)), static_cast<int>(std::round(a.y + depth)), 2, 2, c.marker);
    }
}

void MountainRenderer::drawHorizon(Vector2 camera, float day, float alpha) const
{
    if (alpha <= 0.01f) {
        return;
    }
    constexpr float kHeight = 1.12f;
    drawDistantDuneStrip(camera,
                         0.08f,
                         103,
                         withAlpha(mixColor(Color{31, 49, 55, 255}, Color{63, 126, 112, 255}, day), alpha),
                         withAlpha(mixColor(Color{60, 85, 74, 255}, Color{100, 160, 100, 255}, day), alpha),
                         withAlpha(mixColor(Color{28, 45, 48, 255}, Color{55, 105, 90, 255}, day), alpha * 0.6f),
                         kHeight);
    drawDistantDuneStrip(camera,
                         0.15f,
                         124,
                         withAlpha(mixColor(Color{35, 57, 48, 255}, Color{58, 119, 78, 255}, day), alpha),
                         withAlpha(mixColor(Color{78, 112, 72, 255}, Color{118, 178, 84, 255}, day), alpha),
                         withAlpha(mixColor(Color{32, 52, 42, 255}, Color{50, 100, 65, 255}, day), alpha * 0.6f),
                         kHeight);
}

void MountainRenderer::drawAmbientParticles(Vector2 camera, float day, float alpha, uint32_t seed, float time) const
{
    // Green leaf/grass specks mixed with firefly-like glowing dots.
    // Firefly glow is always visible (day and night), just shifts more yellow after dark.

    // Far layer — grass/leaf bits drifting
    const Color leafColor = mixColor(Color{65, 88, 42, 80}, Color{95, 225, 75, 175}, day);
    biome_detail::drawLoopingParticles(
        seed ^ 0x6666U, time, camera, 0.08f, 7.0f, 10.0f, 0.5f, 18, 1, leafColor, alpha * 0.55f);

    // Near layer — firefly glow dots: flea-bright green, extra luminous at night
    const Color fireflyNight = Color{210, 245, 55, 250};
    const Color fireflyDay = Color{110, 242, 110, 220};
    const Color fireflyColor = mixColor(fireflyNight, fireflyDay, day);
    biome_detail::drawLoopingParticles(
        seed ^ 0x7777U, time, camera, 0.18f, 4.0f, 16.0f, 0.65f, 18, 2, fireflyColor, alpha);
}

} // namespace ridge_dash

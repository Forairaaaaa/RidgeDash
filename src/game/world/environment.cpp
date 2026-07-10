/**
 * @file environment.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/world/environment.hpp"

#include "game/game_config.hpp"
#include "game/render_helpers.hpp"

#include <algorithm>
#include <cmath>

namespace ridge_dash {
namespace {
using namespace game_config;

constexpr float kPi = 3.14159265358979323846f;

uint32_t hash32(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

float random01(uint32_t seed, int index)
{
    return static_cast<float>(hash32(seed + static_cast<uint32_t>(index) * 747796405U) & 0xffffU) / 65535.0f;
}

int wrapToRange(int value, int range)
{
    const int wrapped = value % range;
    return wrapped < 0 ? wrapped + range : wrapped;
}

float smoothstep(float edge0, float edge1, float value)
{
    const float t = game_config::clampf((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

Color mixColor(Color a, Color b, float t)
{
    t = game_config::clampf(t, 0.0f, 1.0f);
    return Color{static_cast<unsigned char>(a.r + (b.r - a.r) * t),
                 static_cast<unsigned char>(a.g + (b.g - a.g) * t),
                 static_cast<unsigned char>(a.b + (b.b - a.b) * t),
                 static_cast<unsigned char>(a.a + (b.a - a.a) * t)};
}

Color withAlpha(Color color, float alpha)
{
    color.a = static_cast<unsigned char>(static_cast<float>(color.a) * game_config::clampf(alpha, 0.0f, 1.0f));
    return color;
}

Vector2 orbitPoint(float progress)
{
    progress = progress - std::floor(progress);
    const float x = -32.0f + progress * (kScreenWidth + 64.0f);
    const float y = 79.0f - std::sin(progress * kPi) * 57.0f;
    return {std::round(x), std::round(y)};
}

bool celestialBlinking(uint32_t seed, float time, int salt)
{
    constexpr float kBlinkPeriod = 5.4f;
    constexpr float kBlinkWindow = 0.055f;
    const float shiftedTime = std::max(0.0f, time + random01(seed, salt) * kBlinkPeriod);
    const int pass = static_cast<int>(std::floor(shiftedTime / kBlinkPeriod));
    const float local = std::fmod(shiftedTime, kBlinkPeriod) / kBlinkPeriod;
    if (local > kBlinkWindow) {
        return false;
    }

    return (hash32(seed + static_cast<uint32_t>(pass) * 977U + static_cast<uint32_t>(salt) * 131U) % 100U) < 82U;
}

void drawStars(uint32_t seed, float distance, float night)
{
    if (night <= 0.02f) {
        return;
    }

    for (int i = 0; i < 34; ++i) {
        const uint32_t starSeed = hash32(seed + static_cast<uint32_t>(i) * 193U);
        const int x = static_cast<int>(starSeed % kScreenWidth);
        const int y = 5 + static_cast<int>((starSeed >> 8U) % 55U);
        const float twinkle = 0.62f + 0.38f * std::sin(distance * 0.035f + static_cast<float>(i) * 1.73f);
        const float alpha = night * twinkle * (0.42f + 0.45f * random01(seed, i + 103));
        const Color color = fadeColor(Color{242, 249, 255, 255}, alpha);

        DrawRectangle(x, y, 1, 1, color);
        if ((starSeed & 0x07U) == 0U) {
            DrawRectangle(x - 1, y, 3, 1, fadeColor(Color{242, 249, 255, 255}, alpha * 0.55f));
            DrawRectangle(x, y - 1, 1, 3, fadeColor(Color{242, 249, 255, 255}, alpha * 0.55f));
        }
    }
}

void drawMeteor(uint32_t seed, float distance, float night)
{
    if (night <= 0.10f) {
        return;
    }

    constexpr float kMeteorPeriod = 165.0f;
    const int pass = static_cast<int>(std::floor(std::max(0.0f, distance) / kMeteorPeriod));
    const uint32_t meteorSeed = hash32(seed ^ (static_cast<uint32_t>(pass) * 0x9e3779b9U));
    if ((meteorSeed % 100U) >= 34U) {
        return;
    }

    const float local = std::fmod(std::max(0.0f, distance), kMeteorPeriod) / kMeteorPeriod;
    if (local > 0.28f) {
        return;
    }

    const float t = local / 0.28f;
    const float fade = night * std::sin(t * kPi);
    const float startX = 34.0f + static_cast<float>((meteorSeed >> 8U) % 180U);
    const float startY = 9.0f + static_cast<float>((meteorSeed >> 17U) % 30U);
    const Vector2 head{std::round(startX + t * 58.0f), std::round(startY + t * 25.0f)};
    const Vector2 tail{head.x - 18.0f, head.y - 8.0f};
    const Color tailColor = fadeColor(Color{210, 238, 255, 255}, fade * 0.55f);
    const Color headColor = fadeColor(Color{250, 255, 255, 255}, fade);

    DrawLineEx(tail, head, 1.0f, tailColor);
    DrawRectangle(static_cast<int>(head.x), static_cast<int>(head.y), 2, 1, headColor);
}

void drawPixelCloud(int x, int y, int variant, Color light, Color shade)
{
    if (variant == 0) {
        DrawRectangle(x, y + 3, 28, 5, light);
        DrawRectangle(x + 5, y, 11, 8, light);
        DrawRectangle(x + 17, y + 1, 8, 7, light);
        DrawRectangle(x + 3, y + 8, 21, 3, shade);
    } else if (variant == 1) {
        DrawRectangle(x + 2, y + 5, 34, 5, light);
        DrawRectangle(x + 8, y + 1, 9, 8, light);
        DrawRectangle(x + 19, y, 12, 9, light);
        DrawRectangle(x, y + 10, 30, 3, shade);
    } else {
        DrawRectangle(x, y + 4, 19, 5, light);
        DrawRectangle(x + 3, y + 1, 8, 7, light);
        DrawRectangle(x + 15, y + 6, 18, 4, light);
        DrawRectangle(x + 4, y + 9, 26, 3, shade);
    }
}

void drawHorizonClouds(float day)
{
    const Color light = mixColor(Color{103, 118, 126, 180}, Color{247, 252, 248, 235}, day);
    const Color mid = mixColor(Color{76, 91, 101, 150}, Color{217, 231, 232, 210}, day);
    const Color shade = mixColor(Color{96, 111, 124, 68}, Color{215, 229, 232, 145}, day);

    DrawRectangle(0, 58, 16, 11, light);
    DrawRectangle(0, 69, 28, 8, light);
    DrawRectangle(6, 49, 12, 14, light);
    DrawRectangle(19, 62, 11, 15, mid);
    DrawRectangle(0, 76, 34, 4, shade);

    DrawRectangle(kScreenWidth - 20, 54, 20, 14, light);
    DrawRectangle(kScreenWidth - 34, 64, 34, 13, light);
    DrawRectangle(kScreenWidth - 44, 72, 44, 8, mid);
    DrawRectangle(kScreenWidth - 29, 47, 14, 14, light);
    DrawRectangle(kScreenWidth - 42, 78, 42, 4, shade);
}

void drawDistantHillStrip(Vector2 camera, float parallax, int baseY, Color mainColor, Color topColor)
{
    const float scroll = camera.x * parallax;
    const int step = 34;
    int start = -step - static_cast<int>(std::fmod(scroll, static_cast<float>(step)));
    if (start > 0) {
        start -= step;
    }

    for (int x = start; x < kScreenWidth + step; x += step) {
        const int peak = baseY - 14 - static_cast<int>((x + static_cast<int>(scroll)) % 3) * 2;
        const int left = x - 16;
        const int mid = x + 16;
        const int right = x + 50;
        for (int px = left; px < right; px += 4) {
            const float t = px < mid ? static_cast<float>(px - left) / static_cast<float>(mid - left)
                                     : static_cast<float>(right - px) / static_cast<float>(right - mid);
            const int y = baseY - static_cast<int>((baseY - peak) * game_config::clampf(t, 0.0f, 1.0f));
            DrawRectangle(px, y, 4, baseY - y, mainColor);
        }
        DrawLineEx(Vector2{static_cast<float>(mid), static_cast<float>(peak)},
                   Vector2{static_cast<float>(x + 28), static_cast<float>(baseY - 5)},
                   1.0f,
                   topColor);
    }
}

void drawDistantDuneStrip(Vector2 camera, float parallax, int baseY, Color mainColor, Color topColor, Color shadowColor)
{
    const float scroll = camera.x * parallax;
    const int step = 46;
    int start = -step - static_cast<int>(std::fmod(scroll, static_cast<float>(step)));
    if (start > 0) {
        start -= step;
    }

    for (int x = start; x < kScreenWidth + step; x += step) {
        const uint32_t duneSeed = hash32(static_cast<uint32_t>(x + static_cast<int>(scroll)) * 97U);
        const int left = x - 18;
        const int right = x + 62;
        const int crest = x + 16 + static_cast<int>(duneSeed % 13U);
        const int height = 12 + static_cast<int>((duneSeed >> 8U) % 8U);
        DrawRectangle(left, baseY - 3, right - left, 7, shadowColor);
        for (int px = left; px < right; px += 4) {
            const float t = static_cast<float>(px - left) / static_cast<float>(right - left);
            const float ridge =
                1.0f - std::abs(static_cast<float>(px - crest) / static_cast<float>(right - left) * 2.0f);
            const float wave = std::sin((t + static_cast<float>((duneSeed >> 16U) & 0xffU) / 255.0f) * kPi) * 0.22f;
            const int y = baseY - static_cast<int>(height * game_config::clampf(ridge + wave, 0.0f, 1.0f));
            DrawRectangle(px, y, 4, baseY + 2 - y, mainColor);
            if (px <= crest + 4 && y < baseY - 4) {
                DrawRectangle(px, y + 2, 4, 1, topColor);
            }
            if (px > crest + 4 && y < baseY - 5) {
                DrawRectangle(px, y + 5, 4, std::max(1, baseY + 2 - (y + 5)), shadowColor);
            }
            if (((px + static_cast<int>(duneSeed)) % 19) < 4 && y < baseY - 8) {
                DrawRectangle(px, y + 6, 3, 1, topColor);
            }
        }
    }
}

void drawDistantStoneStrip(Vector2 camera, float parallax, int baseY, Color mainColor, Color topColor, Color crackColor)
{
    const float scroll = camera.x * parallax;
    const int step = 32;
    int start = -step - static_cast<int>(std::fmod(scroll, static_cast<float>(step)));
    if (start > 0) {
        start -= step;
    }

    for (int x = start; x < kScreenWidth + step; x += step) {
        const uint32_t ridgeSeed = hash32(static_cast<uint32_t>(x + static_cast<int>(scroll)) * 131U);
        const int left = x - 12;
        const int mid = x + 12 + static_cast<int>(ridgeSeed % 10U);
        const int right = x + 42;
        const int peak = baseY - 18 - static_cast<int>((ridgeSeed >> 8U) % 13U);
        for (int px = left; px < right; px += 4) {
            const float t = px < mid ? static_cast<float>(px - left) / static_cast<float>(mid - left)
                                     : static_cast<float>(right - px) / static_cast<float>(right - mid);
            const int y = baseY - static_cast<int>((baseY - peak) * game_config::clampf(t, 0.0f, 1.0f));
            DrawRectangle(px, y, 4, baseY - y, mainColor);
            if (((px + static_cast<int>(ridgeSeed)) % 29) < 4 && y < baseY - 12) {
                DrawRectangle(px, y + 7, 2, std::max(2, (baseY - y) / 2), crackColor);
            }
        }
        DrawLineEx(Vector2{static_cast<float>(mid), static_cast<float>(peak)},
                   Vector2{static_cast<float>(right - 8), static_cast<float>(baseY - 6)},
                   1.0f,
                   topColor);
    }
}

void drawDistantSnowStrip(Vector2 camera, float parallax, int baseY, Color mainColor, Color snowColor, Color shadeColor)
{
    const float scroll = camera.x * parallax;
    const int step = 38;
    int start = -step - static_cast<int>(std::fmod(scroll, static_cast<float>(step)));
    if (start > 0) {
        start -= step;
    }

    for (int x = start; x < kScreenWidth + step; x += step) {
        const uint32_t ridgeSeed = hash32(static_cast<uint32_t>(x + static_cast<int>(scroll)) * 173U);
        const int left = x - 14;
        const int mid = x + 14 + static_cast<int>(ridgeSeed % 8U);
        const int right = x + 50;
        const int peak = baseY - 17 - static_cast<int>((ridgeSeed >> 8U) % 10U);
        for (int px = left; px < right; px += 4) {
            const float t = px < mid ? static_cast<float>(px - left) / static_cast<float>(mid - left)
                                     : static_cast<float>(right - px) / static_cast<float>(right - mid);
            const int y = baseY - static_cast<int>((baseY - peak) * game_config::clampf(t, 0.0f, 1.0f));
            DrawRectangle(px, y, 4, baseY - y, mainColor);
            if (px >= mid && y < baseY - 8) {
                DrawRectangle(px, y + 5, 4, std::max(2, (baseY - y) / 3), shadeColor);
            }
            if (y < baseY - 10) {
                DrawRectangle(px, y, 4, std::min(5, baseY - y), snowColor);
            }
        }
    }
}

void drawBiomeHorizon(Vector2 camera, float day, TerrainBiome biome, float alpha)
{
    if (alpha <= 0.01f) {
        return;
    }

    if (biome == TerrainBiome::Desert) {
        drawDistantDuneStrip(camera,
                             0.08f,
                             103,
                             withAlpha(mixColor(Color{78, 56, 41, 255}, Color{199, 146, 72, 255}, day), alpha),
                             withAlpha(mixColor(Color{122, 88, 54, 255}, Color{239, 196, 103, 255}, day), alpha),
                             withAlpha(mixColor(Color{72, 52, 42, 255}, Color{187, 126, 66, 255}, day), alpha));
        drawDistantDuneStrip(camera,
                             0.15f,
                             124,
                             withAlpha(mixColor(Color{88, 59, 41, 255}, Color{221, 162, 78, 255}, day), alpha),
                             withAlpha(mixColor(Color{136, 91, 54, 255}, Color{255, 208, 103, 255}, day), alpha),
                             withAlpha(mixColor(Color{77, 51, 39, 255}, Color{202, 129, 63, 255}, day), alpha));
        return;
    }

    if (biome == TerrainBiome::Stone) {
        drawDistantStoneStrip(camera,
                              0.08f,
                              103,
                              withAlpha(mixColor(Color{34, 39, 45, 255}, Color{93, 102, 108, 255}, day), alpha),
                              withAlpha(mixColor(Color{76, 83, 91, 255}, Color{156, 166, 171, 255}, day), alpha),
                              withAlpha(mixColor(Color{42, 47, 54, 255}, Color{101, 108, 116, 255}, day), alpha));
        drawDistantStoneStrip(camera,
                              0.15f,
                              124,
                              withAlpha(mixColor(Color{41, 45, 50, 255}, Color{111, 118, 123, 255}, day), alpha),
                              withAlpha(mixColor(Color{87, 94, 101, 255}, Color{177, 184, 188, 255}, day), alpha),
                              withAlpha(mixColor(Color{48, 52, 59, 255}, Color{116, 122, 130, 255}, day), alpha));
        return;
    }

    if (biome == TerrainBiome::Snow) {
        drawDistantSnowStrip(camera,
                             0.08f,
                             103,
                             withAlpha(mixColor(Color{35, 55, 69, 255}, Color{95, 144, 165, 255}, day), alpha),
                             withAlpha(mixColor(Color{186, 212, 224, 255}, Color{245, 253, 255, 255}, day), alpha),
                             withAlpha(mixColor(Color{58, 79, 94, 255}, Color{143, 181, 198, 255}, day), alpha));
        drawDistantSnowStrip(camera,
                             0.15f,
                             124,
                             withAlpha(mixColor(Color{43, 64, 76, 255}, Color{110, 158, 177, 255}, day), alpha),
                             withAlpha(mixColor(Color{196, 222, 232, 255}, Color{250, 255, 255, 255}, day), alpha),
                             withAlpha(mixColor(Color{66, 88, 101, 255}, Color{156, 194, 209, 255}, day), alpha));
        return;
    }

    drawDistantHillStrip(camera,
                         0.08f,
                         103,
                         withAlpha(mixColor(Color{31, 49, 55, 255}, Color{63, 126, 112, 255}, day), alpha),
                         withAlpha(mixColor(Color{60, 85, 74, 255}, Color{100, 160, 100, 255}, day), alpha));
    drawDistantHillStrip(camera,
                         0.15f,
                         124,
                         withAlpha(mixColor(Color{35, 57, 48, 255}, Color{58, 119, 78, 255}, day), alpha),
                         withAlpha(mixColor(Color{78, 112, 72, 255}, Color{118, 178, 84, 255}, day), alpha));
}

void drawSeaLinePattern(int x, int y, int width, Color color)
{
    DrawRectangle(x, y, width, 1, color);
}

void drawSeaTexture(Vector2 camera, float day)
{
    const int offset = static_cast<int>(std::round(camera.x * 0.035f)) % 46;
    const Color horizon = mixColor(Color{35, 74, 88, 255}, Color{0, 94, 184, 255}, day);
    const Color light = mixColor(Color{74, 118, 130, 135}, Color{132, 203, 250, 168}, day);
    const Color mid = mixColor(Color{38, 83, 101, 180}, Color{7, 112, 202, 216}, day);
    const Color dark = mixColor(Color{26, 58, 75, 170}, Color{5, 64, 150, 226}, day);
    const Color foam = mixColor(Color{145, 171, 176, 120}, Color{211, 239, 255, 168}, day);

    DrawRectangle(0, 68, kScreenWidth, 2, horizon);
    DrawRectangle(0, 70, kScreenWidth, 24, mixColor(Color{24, 30, 39, 255}, Color{3, 113, 205, 255}, day));
    DrawRectangle(
        0, 94, kScreenWidth, kScreenHeight - 94, mixColor(Color{18, 26, 35, 255}, Color{13, 82, 176, 255}, day));

    static constexpr int kLineData[][4] = {
        {18, 78, 38, 0},
        {82, 80, 26, 1},
        {141, 78, 48, 0},
        {226, 82, 34, 1},
        {31, 92, 24, 2},
        {71, 97, 68, 0},
        {172, 96, 42, 1},
        {238, 99, 55, 0},
        {12, 111, 50, 1},
        {85, 116, 28, 2},
        {133, 113, 77, 0},
        {226, 119, 31, 1},
        {42, 129, 71, 0},
        {152, 133, 34, 2},
        {213, 130, 64, 1},
    };

    for (const auto& line : kLineData) {
        const int x = (line[0] - offset * (line[3] + 1) + kScreenWidth * 2) % (kScreenWidth + 70) - 35;
        const Color color = line[3] == 0 ? light : (line[3] == 1 ? mid : dark);
        drawSeaLinePattern(x, line[1], line[2], color);
    }

    for (int i = 0; i < 12; ++i) {
        const int x = (i * 29 + 11 - offset * 2 + kScreenWidth * 2) % kScreenWidth;
        const int y = 83 + (i * 17) % 47;
        DrawRectangle(x, y, 2 + (i % 3), 1, foam);
    }
}

void drawSailboats(const Texture2D& sailboat, Vector2 camera, float day, uint32_t seed)
{
    if (!textureLoaded(sailboat) || day <= 0.04f) {
        return;
    }

    const int wrap = kScreenWidth + 128;
    const int count = 1 + static_cast<int>(hash32(seed ^ 0x51A1B0A7U) % 3U);
    const Color tint = fadeColor(WHITE, game_config::clampf(day * 1.2f, 0.0f, 1.0f));

    for (int i = 0; i < count; ++i) {
        const uint32_t boatSeed = hash32(seed + static_cast<uint32_t>(i) * 0x9E3779B9U);
        const int baseX = 72 + i * 128 + static_cast<int>((boatSeed >> 5U) % 52U);
        const int x = wrapToRange(baseX - static_cast<int>(std::round(camera.x * (0.045f + i * 0.006f))), wrap) - 64;
        const int y = 78 + static_cast<int>((boatSeed >> 17U) % 13U);
        drawSpriteCentered(sailboat, Vector2{static_cast<float>(x), static_cast<float>(y)}, 12.0f, 9.0f, 0.0f, tint);
    }
}

} // namespace

void Environment::loadAssets()
{
    _sun = loadSpriteTexture("sun.png");
    _sunBlink = loadSpriteTexture("sun_blink.png");
    _moon = loadSpriteTexture("moon.png");
    _moonBlink = loadSpriteTexture("moon_blink.png");
    _sailboat = loadSpriteTexture("sailboat.png");
}

void Environment::unloadAssets()
{
    unloadSpriteTexture(_sun);
    unloadSpriteTexture(_sunBlink);
    unloadSpriteTexture(_moon);
    unloadSpriteTexture(_moonBlink);
    unloadSpriteTexture(_sailboat);
}

void Environment::reset(uint32_t seed)
{
    _seed = seed;
    _fromBiome = TerrainBiome::Mountain;
    _targetBiome = TerrainBiome::Mountain;
    _biomeBlend = 1.0f;
    _time = 0.0f;
}

void Environment::updateBiome(TerrainBiome targetBiome, float dt)
{
    constexpr float kBiomeFadeDuration = 0.34f;
    _time += dt;
    if (targetBiome != _targetBiome) {
        _fromBiome = _biomeBlend >= 0.5f ? _targetBiome : _fromBiome;
        _targetBiome = targetBiome;
        _biomeBlend = 0.0f;
    }

    if (_fromBiome == _targetBiome) {
        _biomeBlend = 1.0f;
        return;
    }

    _biomeBlend = std::min(1.0f, _biomeBlend + dt / kBiomeFadeDuration);
    if (_biomeBlend == 1.0f) {
        _fromBiome = _targetBiome;
    }
}

float Environment::dayAmount(float distance) const
{
    constexpr float kCycleDistance = 600.0f;
    constexpr float kStartPhase = 0.3167f;
    constexpr float kTwoPi = kPi * 2.0f;
    const float phase = std::fmod(kStartPhase + std::max(0.0f, distance) / kCycleDistance, 1.0f);
    const float sunAltitude = std::sin(phase * kTwoPi - kPi * 0.5f);
    return smoothstep(-0.12f, 0.46f, sunAltitude);
}

void Environment::drawBackground(Vector2 camera, float distance) const
{
    constexpr float kCycleDistance = 600.0f;
    constexpr float kStartPhase = 0.3167f;
    constexpr float kTwoPi = kPi * 2.0f;
    const float phase = std::fmod(kStartPhase + std::max(0.0f, distance) / kCycleDistance, 1.0f);
    const float day = dayAmount(distance);
    const float night = 1.0f - day;

    ClearBackground(mixColor(Color{15, 20, 30, 255}, Color{52, 142, 170, 255}, day));
    DrawRectangle(0, 0, kScreenWidth, 34, mixColor(Color{70, 100, 126, 255}, Color{85, 192, 220, 255}, day));
    DrawRectangle(0, 34, kScreenWidth, 34, mixColor(Color{49, 75, 99, 255}, Color{77, 166, 195, 255}, day));
    drawStars(_seed, distance, night);
    drawMeteor(_seed, distance, night);

    const float sunProgress = game_config::clampf((phase - 0.25f) / 0.5f, 0.0f, 1.0f);
    const Vector2 sun = orbitPoint(sunProgress);
    if (day > 0.02f) {
        const bool blink = celestialBlinking(_seed, _time, 17) && textureLoaded(_sunBlink);
        const Texture2D& sunTexture = blink ? _sunBlink : _sun;
        if (textureLoaded(sunTexture)) {
            drawSpriteCentered(sunTexture, sun, 28.0f, 28.0f, 0.0f, fadeColor(WHITE, day));
        } else {
            DrawRectangle(static_cast<int>(sun.x - 8.0f),
                          static_cast<int>(sun.y - 8.0f),
                          17,
                          17,
                          fadeColor(Color{245, 204, 105, 255}, day));
            DrawRectangle(static_cast<int>(sun.x + 9.0f),
                          static_cast<int>(sun.y - 3.0f),
                          5,
                          8,
                          fadeColor(Color{245, 204, 105, 255}, day));
        }
    }

    const Vector2 moon = orbitPoint(phase + 0.5f);
    if (night > 0.02f) {
        const bool blink = celestialBlinking(_seed, _time, 43) && textureLoaded(_moonBlink);
        const Texture2D& moonTexture = blink ? _moonBlink : _moon;
        if (textureLoaded(moonTexture)) {
            drawSpriteCentered(moonTexture, moon, 24.0f, 20.0f, 0.0f, fadeColor(WHITE, night));
        } else {
            DrawRectangle(static_cast<int>(moon.x - 7.0f),
                          static_cast<int>(moon.y - 6.0f),
                          15,
                          13,
                          fadeColor(Color{216, 226, 232, 255}, night));
            DrawRectangle(static_cast<int>(moon.x + 2.0f),
                          static_cast<int>(moon.y - 8.0f),
                          6,
                          6,
                          fadeColor(Color{15, 20, 30, 255}, night));
        }
    }

    drawHorizonClouds(day);
    drawSeaTexture(camera, day);
    drawSailboats(_sailboat, camera, day, _seed);
    if (_fromBiome != _targetBiome && _biomeBlend < 1.0f) {
        drawBiomeHorizon(camera, day, _fromBiome, 1.0f - _biomeBlend);
        drawBiomeHorizon(camera, day, _targetBiome, _biomeBlend);
    } else {
        drawBiomeHorizon(camera, day, _targetBiome, 1.0f);
    }

    for (int i = 0; i < 8; ++i) {
        const float cloudWorldX = i * 72.0f + random01(_seed, i) * 46.0f;
        const int x = wrapToRange(static_cast<int>(std::round(cloudWorldX - camera.x * 0.18f)), kScreenWidth + 96) - 48;
        const int y = 17 + static_cast<int>(random01(_seed, i + 31) * 25.0f);
        const int variant = static_cast<int>(hash32(_seed + static_cast<uint32_t>(i) * 97U) % 3U);
        const Color light = mixColor(Color{103, 118, 126, 160}, Color{247, 252, 248, 225}, day);
        const Color shade = mixColor(Color{96, 111, 124, 60}, Color{220, 234, 237, 132}, day);
        drawPixelCloud(x, y, variant, light, shade);
    }
}

} // namespace ridge_dash

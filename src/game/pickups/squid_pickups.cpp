/**
 * @file squid_pickups.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/pickups/pickups.hpp"

#include "game/game_config.hpp"
#include "game/ridge_dash_game.hpp"
#include "game/render_helpers.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

namespace ridge_dash {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = kPi * 2.0f;
constexpr float kSquidWidth = 32.0f;
constexpr float kSquidHeight = 35.0f;

float clamp01(float value)
{
    return std::max(0.0f, std::min(value, 1.0f));
}

float lerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}

float hash01(uint32_t value)
{
    return static_cast<float>(value & 0xffU) / 255.0f;
}

Color squidColor(int sprite)
{
    static constexpr Color kColors[] = {
        Color{112, 104, 203, 255},
        Color{18, 148, 118, 255},
        Color{180, 28, 76, 255},
        Color{163, 97, 21, 255},
    };
    return kColors[static_cast<unsigned>(sprite) % 4U];
}

Color lighten(Color color, int amount)
{
    color.r = static_cast<unsigned char>(std::min(255, static_cast<int>(color.r) + amount));
    color.g = static_cast<unsigned char>(std::min(255, static_cast<int>(color.g) + amount));
    color.b = static_cast<unsigned char>(std::min(255, static_cast<int>(color.b) + amount));
    return color;
}

} // namespace

using namespace game_config;

void SquidPickups::clear()
{
    _squids.clear();
    _trail.clear();
    _trailRemainder = 0.0f;
    _serial = 0;
}

void SquidPickups::reset()
{
    clear();
    _squids.reserve(4);
    _trail.reserve(128);
}

void SquidPickups::trigger(uint32_t runSeed)
{
    _squids.clear();
    _trail.clear();
    _trailRemainder = 0.0f;

    std::mt19937 rng(runSeed * 811u + _serial++ * 31337u + 0x51u);
    std::array<int, 4> spriteOrder = {0, 1, 2, 3};
    std::array<int, 4> laneOrder = {0, 1, 2, 3};
    std::shuffle(spriteOrder.begin(), spriteOrder.end(), rng);
    std::shuffle(laneOrder.begin(), laneOrder.end(), rng);

    std::uniform_real_distribution<float> jitter(-4.0f, 4.0f);
    std::uniform_real_distribution<float> phaseDist(0.0f, kTwoPi);
    std::uniform_real_distribution<float> arcDist(30.0f, 44.0f);
    std::uniform_real_distribution<float> durationDist(2.05f, 2.55f);
    std::uniform_real_distribution<float> startDist(0.0f, 28.0f);
    const float centerY = 82.0f + jitter(rng) * 1.5f;

    for (int i = 0; i < 4; ++i) {
        Squid squid{};
        const float lane = static_cast<float>(laneOrder[static_cast<size_t>(i)]) - 1.5f;
        squid.sprite = spriteOrder[static_cast<size_t>(i)];
        squid.baseY = centerY + lane * 25.0f + jitter(rng);
        squid.start = {-46.0f - startDist(rng), squid.baseY};
        squid.end = {static_cast<float>(kScreenWidth) + 54.0f + startDist(rng), squid.baseY + jitter(rng)};
        squid.duration = durationDist(rng);
        squid.arc = -arcDist(rng) + lane * 2.5f;
        squid.phase = phaseDist(rng);
        squid.seed = runSeed * 101u + static_cast<uint32_t>(i) * 9176u + _serial * 13u;
        _squids.push_back(squid);
    }
}

Vector2 SquidPickups::position(const Squid& squid)
{
    const float t = clamp01(squid.age / squid.duration);
    const float x = lerpf(squid.start.x, squid.end.x, t);
    const float y = squid.baseY + std::sin(t * kPi) * squid.arc + std::sin(t * kTwoPi + squid.phase) * 2.5f;
    return {x, y};
}

void SquidPickups::spawnTrail(const Squid& squid)
{
    const Vector2 pos = position(squid);
    const uint32_t seed = squid.seed + _serial++ * 0x9E3779B9U;
    const float r0 = hash01(seed);
    const float r1 = hash01(seed >> 8U);
    const float r2 = hash01(seed >> 16U);

    const Color base = squidColor(squid.sprite);
    const Color color = r0 < 0.30f ? lighten(base, 80) : (r0 < 0.72f ? base : lighten(base, 124));

    TrailParticle particle{};
    particle.pos = {pos.x - kSquidWidth * 0.48f + (r1 - 0.5f) * 4.5f, pos.y + (r2 - 0.5f) * 9.0f};
    particle.vel = {-38.0f - r0 * 28.0f, (r1 - 0.5f) * 30.0f};
    particle.age = 0.0f;
    particle.life = 0.34f + r2 * 0.24f;
    particle.color = color;
    particle.seed = seed;
    _trail.push_back(particle);
}

void SquidPickups::update(float dt)
{
    auto squidIt = _squids.begin();
    while (squidIt != _squids.end()) {
        squidIt->age += dt;
        if (squidIt->age >= squidIt->duration) {
            squidIt = _squids.erase(squidIt);
        } else {
            _trailRemainder += dt * 20.0f;
            while (_trailRemainder >= 1.0f) {
                spawnTrail(*squidIt);
                _trailRemainder -= 1.0f;
            }
            ++squidIt;
        }
    }

    auto trailIt = _trail.begin();
    while (trailIt != _trail.end()) {
        trailIt->age += dt;
        trailIt->pos.x += trailIt->vel.x * dt;
        trailIt->pos.y += trailIt->vel.y * dt;
        trailIt->vel.y += 18.0f * dt;
        trailIt->vel.x *= std::max(0.0f, 1.0f - 1.2f * dt);
        if (trailIt->age >= trailIt->life) {
            trailIt = _trail.erase(trailIt);
        } else {
            ++trailIt;
        }
    }
}

void SquidPickups::draw(const RidgeDashGame& game) const
{
    for (const TrailParticle& particle : _trail) {
        const float t = particle.age / particle.life;
        const float alpha = std::max(0.0f, 1.0f - t * t);
        const int size = t < 0.22f ? 6 : (t < 0.62f ? 4 : 3);
        const Color color = fadeColor(particle.color, alpha);
        DrawRectangle(static_cast<int>(std::round(particle.pos.x)) - size / 2,
                      static_cast<int>(std::round(particle.pos.y)) - size / 2,
                      size,
                      size,
                      color);
    }

    for (const Squid& squid : _squids) {
        const Vector2 pos = position(squid);
        if (pos.x < -64.0f || pos.x > kScreenWidth + 64.0f || pos.y < -48.0f || pos.y > kScreenHeight + 48.0f) {
            continue;
        }

        const float t = clamp01(squid.age / squid.duration);
        const float rotation =
            std::sin(t * kTwoPi * 1.35f + squid.phase) * 8.0f + std::cos(t * kPi) * squid.arc * 0.32f;
        const Texture2D& texture = squid.sprite == 0   ? game._sprites.squidA
                                   : squid.sprite == 1 ? game._sprites.squidB
                                   : squid.sprite == 2 ? game._sprites.squidC
                                                       : game._sprites.squidD;
        if (textureLoaded(texture)) {
            drawSpriteCentered(texture,
                               {std::round(pos.x), std::round(pos.y)},
                               static_cast<float>(texture.width),
                               static_cast<float>(texture.height),
                               rotation);
        } else {
            const Color color = squidColor(squid.sprite);
            DrawRectanglePro(
                Rectangle{std::round(pos.x), std::round(pos.y), 28.0f, 24.0f}, Vector2{14.0f, 12.0f}, rotation, color);
        }
    }
}

} // namespace ridge_dash

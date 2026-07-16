/**
 * @file pickup_effects.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/pickups/pickups.hpp"

#include "game/game_config.hpp"
#include "game/ridge_dash_game.hpp"
#include "game/pickups/pickup_base_impl.hpp"
#include "game/render_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ridge_dash {
namespace {

float clamp01(float value)
{
    return std::max(0.0f, std::min(value, 1.0f));
}

float smoothstep(float value)
{
    value = clamp01(value);
    return value * value * (3.0f - 2.0f * value);
}

float lerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}

Vector2 lerpVector(Vector2 a, Vector2 b, float t)
{
    return {lerpf(a.x, b.x, t), lerpf(a.y, b.y, t)};
}

} // namespace

using namespace game_config;

void PickupEffects::clear()
{
    _puffs.clear();
}

void PickupEffects::reset()
{
    _puffs.clear();
    _serial = 0;
}

void PickupEffects::spawn(Vector2 pos, Kind kind, uint32_t runSeed)
{
    const float life = kind == Kind::Fuel        ? 1.04f
                       : kind == Kind::Flea      ? 0.56f
                       : kind == Kind::Rocket    ? 0.62f
                       : kind == Kind::Cactus    ? 0.58f
                       : kind == Kind::Snowman   ? 0.72f
                       : kind == Kind::GiantFlea ? 0.64f
                       : kind == Kind::Helmet    ? 0.68f
                       : kind == Kind::Magnet    ? 0.76f
                                                 : 0.96f;
    _puffs.push_back(Puff{pos, 0.0f, life, runSeed * 97u + _serial++ * 31u, kind});
}

void PickupEffects::update(float dt)
{
    auto it = _puffs.begin();
    while (it != _puffs.end()) {
        it->age += dt;
        if (it->age >= it->life) {
            it = _puffs.erase(it);
        } else {
            ++it;
        }
    }
}

void PickupEffects::draw(const RidgeDashGame& game) const
{
    const Color fuelColors[] = {
        Color{178, 255, 190, 255},
        Color{255, 255, 194, 255},
        Color{255, 116, 94, 255},
    };
    const Color coinColors[] = {
        Color{255, 244, 102, 255},
        Color{255, 255, 214, 255},
        Color{255, 194, 64, 255},
    };
    const Color fleaColors[] = {
        Color{255, 255, 255, 255},
        Color{240, 242, 246, 255},
        Color{215, 220, 230, 255},
    };
    const Color rocketColors[] = {
        Color{255, 233, 104, 255},
        Color{255, 124, 92, 255},
        Color{255, 247, 212, 255},
    };
    const Color cactusColors[] = {
        Color{150, 255, 130, 255},
        Color{255, 242, 144, 255},
        Color{73, 198, 95, 255},
    };
    const Color snowmanColors[] = {
        Color{242, 253, 255, 255},
        Color{168, 220, 238, 255},
        Color{255, 130, 78, 255},
    };
    // GiantFlea uses the same white/grey dust as Flea but more particles.
    const Color giantFleaColors[] = {
        Color{255, 255, 255, 255},
        Color{240, 242, 246, 255},
        Color{215, 220, 230, 255},
    };
    const Color helmetColors[] = {
        Color{255, 255, 180, 255},
        Color{200, 200, 220, 255},
        Color{255, 220, 80, 255},
    };
    const Color magnetColors[] = {
        Color{220, 50, 50, 255},   // red (N pole)
        Color{50, 50, 220, 255},   // blue (S pole)
        Color{255, 140, 140, 255}, // light red highlight
    };

    for (const Puff& puff : _puffs) {
        const float t = puff.age / puff.life;
        const bool isFuel = puff.kind == Kind::Fuel;
        const bool isFlea = puff.kind == Kind::Flea;
        const bool isRocket = puff.kind == Kind::Rocket;
        const bool isCactus = puff.kind == Kind::Cactus;
        const bool isSnowman = puff.kind == Kind::Snowman;
        const bool isGiantFlea = puff.kind == Kind::GiantFlea;
        const bool isHelmet = puff.kind == Kind::Helmet;
        const bool isMagnet = puff.kind == Kind::Magnet;
        const bool gathersToHud = isFuel || puff.kind == Kind::Coin;
        const float alpha = !gathersToHud ? (t < 0.58f ? 1.0f : std::max(0.0f, 1.0f - (t - 0.58f) / 0.42f))
                                          : (t < 0.84f ? 1.0f : std::max(0.0f, 1.0f - (t - 0.84f) / 0.16f));
        const int count =
            isFuel
                ? 22
                : (isFlea ? 24
                          : (isRocket
                                 ? 26
                                 : (isCactus
                                        ? 20
                                        : (isSnowman ? 28
                                                     : (isGiantFlea ? 32 : (isHelmet ? 22 : (isMagnet ? 26 : 18)))))));
        const float speed =
            isFuel
                ? 28.0f
                : (isFlea ? 31.0f
                          : (isRocket
                                 ? 34.0f
                                 : (isCactus ? 26.0f
                                             : (isSnowman ? 30.0f
                                                          : (isGiantFlea
                                                                 ? 36.0f
                                                                 : (isHelmet ? 26.0f : (isMagnet ? 30.0f : 22.0f)))))));
        const float gravity =
            isFuel
                ? 27.0f
                : (isFlea ? 26.0f
                          : (isRocket
                                 ? 24.0f
                                 : (isCactus ? 30.0f
                                             : (isSnowman ? 33.0f
                                                          : (isGiantFlea
                                                                 ? 28.0f
                                                                 : (isHelmet ? 25.0f : (isMagnet ? 28.0f : 22.0f)))))));
        const Vector2 center = game.worldToScreen({puff.pos.x, puff.pos.y});
        const Vector2 hudTarget =
            isFuel ? Vector2{49.0f, static_cast<float>(kScreenHeight - 15)}
                   : Vector2{static_cast<float>(kScreenWidth - 18), static_cast<float>(kScreenHeight - 8)};

        if (center.x < -24.0f || center.x > kScreenWidth + 24.0f || center.y < -24.0f ||
            center.y > kScreenHeight + 24.0f) {
            continue;
        }

        for (int i = 0; i < count; ++i) {
            const uint32_t dotSeed = puff.seed + static_cast<uint32_t>(i) * 0x9E3779B9U;
            const float r0 = static_cast<float>(dotSeed & 0xffU) / 255.0f;
            const float r1 = static_cast<float>((dotSeed >> 8U) & 0xffU) / 255.0f;
            const float r2 = static_cast<float>((dotSeed >> 16U) & 0xffU) / 255.0f;
            const float r3 = static_cast<float>((dotSeed >> 24U) & 0xffU) / 255.0f;
            const float angleJitter = static_cast<float>(dotSeed & 0xffU) / 255.0f * 0.72f;
            const float speedJitter = 0.58f + r1 * 0.86f;
            const float riseJitter = r2 * 8.0f;
            const float angle = static_cast<float>(i) * 6.2831853f / static_cast<float>(count) + angleJitter;
            const float vx = std::cos(angle) * speed * speedJitter;
            const float vy = std::sin(angle) * speed * 0.72f * speedJitter - 9.0f - riseJitter;
            const float burstT =
                (isFlea || isGiantFlea) ? std::min(t, 0.32f + r3 * 0.24f) : std::min(t, 0.44f + r3 * 0.08f);
            Vector2 pos = {center.x + vx * burstT, center.y + vy * burstT + gravity * burstT * burstT};
            if (gathersToHud) {
                const float gatherStart = 0.36f + r0 * 0.20f;
                const float gatherDuration = 0.42f + r1 * 0.30f;
                const float gather = smoothstep((t - gatherStart) / gatherDuration);
                const float maxGather = 0.82f + r2 * 0.12f;
                const float pull = gather * maxGather;
                const float targetSpread = lerpf(isFuel ? 18.0f : 22.0f, isFuel ? 5.5f : 7.5f, gather);
                const float targetJitterX = (r0 - 0.5f) * targetSpread;
                const float targetJitterY = (r2 - 0.5f) * targetSpread * 0.46f;
                const float flutter = std::sin((t * (7.0f + r1 * 5.0f) + r3 * 6.2831853f)) * (1.0f - gather) * 3.5f;
                Vector2 target = {hudTarget.x + targetJitterX, hudTarget.y + targetJitterY};
                pos = lerpVector(pos, target, pull);
                pos.x += flutter;
                pos.y -= std::sin(gather * 3.1415926f) * (isFuel ? 8.0f : 10.0f);
            }

            const int size = t < 0.20f ? 5 : (t < 0.52f ? 4 : (t < 0.84f ? 3 : 2));
            const Color base = isFuel        ? fuelColors[i % 3]
                               : isFlea      ? fleaColors[i % 3]
                               : isRocket    ? rocketColors[i % 3]
                               : isCactus    ? cactusColors[i % 3]
                               : isSnowman   ? snowmanColors[i % 3]
                               : isGiantFlea ? giantFleaColors[i % 3]
                               : isHelmet    ? helmetColors[i % 3]
                               : isMagnet    ? magnetColors[i % 3]
                                             : coinColors[i % 3];
            const Color color = fadeColor(base, alpha);
            const int x = static_cast<int>(std::round(pos.x)) - size / 2;
            const int y = static_cast<int>(std::round(pos.y)) - size / 2;
            DrawRectangle(x, y, size, size, color);
        }
    }
}

} // namespace ridge_dash

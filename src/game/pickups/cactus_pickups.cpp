/**
 * @file cactus_pickups.cpp
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
#include "game/render_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace ridge_dash {
namespace {

float hash01(uint32_t value)
{
    return static_cast<float>(value & 0xffU) / 255.0f;
}

void drawPixelCactus(int ix, int baseY, int bend, Color dark, Color body, Color light, Color spine)
{
    DrawRectangle(ix - 6 + bend, baseY - 23, 12, 22, dark);
    DrawRectangle(ix - 4 + bend, baseY - 25, 8, 3, dark);
    DrawRectangle(ix - 4 + bend, baseY - 22, 8, 21, body);
    DrawRectangle(ix - 1 + bend, baseY - 21, 3, 18, light);

    DrawRectangle(ix - 14 + bend / 2, baseY - 16, 10, 5, dark);
    DrawRectangle(ix - 14 + bend / 2, baseY - 22, 4, 11, dark);
    DrawRectangle(ix - 13 + bend / 2, baseY - 20, 2, 8, body);
    DrawRectangle(ix - 11 + bend / 2, baseY - 15, 7, 3, body);

    DrawRectangle(ix + 4 + bend / 2, baseY - 13, 11, 5, dark);
    DrawRectangle(ix + 11 + bend / 2, baseY - 20, 4, 12, dark);
    DrawRectangle(ix + 12 + bend / 2, baseY - 18, 2, 9, body);
    DrawRectangle(ix + 4 + bend / 2, baseY - 12, 9, 3, body);

    DrawRectangle(ix - 8, baseY - 1, 7, 2, dark);
    DrawRectangle(ix + 1, baseY - 1, 7, 2, dark);
    DrawRectangle(ix - 7 + bend, baseY - 18, 1, 1, spine);
    DrawRectangle(ix + 6 + bend, baseY - 11, 1, 1, spine);
    DrawRectangle(ix - 15 + bend / 2, baseY - 17, 1, 1, spine);
    DrawRectangle(ix + 16 + bend / 2, baseY - 13, 1, 1, spine);
}

} // namespace

using namespace game_config;

void CactusPickups::doClear()
{
    _chips.clear();
    _chipSerial = 0;
}

void CactusPickups::doTrimExtra(float minX)
{
    auto chipIt = _chips.begin();
    while (chipIt != _chips.end()) {
        if (chipIt->pos.x >= minX) {
            ++chipIt;
        } else {
            chipIt = _chips.erase(chipIt);
        }
    }
}

void CactusPickups::reset(RidgeDashGame& game)
{
    clear();
    _items.reserve(16);
    _chips.reserve(80);

    std::uniform_real_distribution<float> gapDist(22.0f, 42.0f);
    _nextX = 74.0f + gapDist(game._rng);
    stream(game, game._startX + kCactusGenerateAhead);
}

void CactusPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(22.0f, 42.0f);
    while (_nextX < targetX) {
        const TerrainSample terrain = game._terrain.sampleAt(_nextX, 12.0f, game._rng);

        if (terrain.biome != TerrainBiome::Desert) {
            _nextX += 8.0f;
            continue;
        }
        if (game._pickups.fuel().activeNear(_nextX, 5.2f) || game._pickups.coin().activeNear(_nextX, 4.4f) ||
            game._pickups.flea().activeInRange(_nextX - 5.0f, _nextX + 5.0f) ||
            game._pickups.rocket().activeNear(_nextX, 6.0f)) {
            _nextX += 6.6f;
            continue;
        }

        doCreate(game, terrain);
        _nextX += gapDist(game._rng);
    }
}

void CactusPickups::doCreate(RidgeDashGame& game, const TerrainSample& terrain)
{
    if (!b2World_IsValid(game._worldId)) {
        return;
    }

    _items.push_back(Item{});
    Item& cactus = _items.back();
    cactus.pos = {terrain.x, terrain.y - 0.93f};

    createSensorBody(game, cactus.pos, kCactusRadius, cactus.bodyId, cactus.shapeId);
}

void CactusPickups::spawnChips(RidgeDashGame& game, Vector2 pos, float direction)
{
    for (int i = 0; i < 10; ++i) {
        const uint32_t seed = game._runSeed * 131u + _chipSerial++ * 2654435761u;
        const float r0 = hash01(seed);
        const float r1 = hash01(seed >> 8U);
        const float r2 = hash01(seed >> 16U);

        Chip chip{};
        chip.pos = pos;
        chip.vel = {direction * (1.0f + r0 * 3.2f) + (r1 - 0.5f) * 1.6f, -2.4f - r2 * 3.1f};
        chip.age = 0.0f;
        chip.life = 0.32f + r1 * 0.22f;
        chip.seed = seed;
        _chips.push_back(chip);
    }
}

bool CactusPickups::doCollect(RidgeDashGame& game, Item& cactus)
{
    if (!cactus.active || cactus.cooldown > 0.0f || !game.carValid()) {
        return false;
    }

    const b2Vec2 velocity = game._vehicle.chassisVelocity();
    const float direction = velocity.x >= 0.0f ? -1.0f : 1.0f;
    const float slowX = direction * clampf(1.2f + std::abs(velocity.x) * 0.42f, 1.2f, 6.2f);
    game._vehicle.applyMainBodyDeltaVelocity({slowX, -0.26f}, 1.0f, 0.65f, 0.65f);
    game._vehicle.applyChassisAngularImpulse(direction * 0.22f);

    cactus.cooldown = 0.78f;
    cactus.wobbleVelocity += -direction * (16.0f + std::min(std::abs(velocity.x), 12.0f));
    spawnChips(game, {cactus.pos.x, cactus.pos.y - 0.22f}, -direction);
    game._pickups.effects().spawn({cactus.pos.x, cactus.pos.y - 0.22f}, PickupEffects::Kind::Cactus, game._runSeed);
    game.playSfx(AudioSystem::Sfx::Cactus);
    return true;
}

float CactusPickups::pickupDistance() const
{
    return kCactusPickupDistance;
}

void CactusPickups::update(float dt)
{
    for (Item& cactus : _items) {
        if (!cactus.active) {
            continue;
        }
        cactus.cooldown = std::max(0.0f, cactus.cooldown - dt);
        cactus.wobbleVelocity += -cactus.wobble * 42.0f * dt;
        cactus.wobbleVelocity *= std::pow(0.06f, dt);
        cactus.wobble += cactus.wobbleVelocity * dt;
        cactus.wobble = clampf(cactus.wobble, -1.0f, 1.0f);
    }

    auto chipIt = _chips.begin();
    while (chipIt != _chips.end()) {
        chipIt->age += dt;
        chipIt->pos.x += chipIt->vel.x * dt;
        chipIt->pos.y += chipIt->vel.y * dt;
        chipIt->vel.y += 8.6f * dt;
        chipIt->vel.x *= std::max(0.0f, 1.0f - 1.8f * dt);
        if (chipIt->age >= chipIt->life) {
            chipIt = _chips.erase(chipIt);
        } else {
            ++chipIt;
        }
    }
}

void CactusPickups::draw(const RidgeDashGame& game) const
{
    const Color chipColors[] = {
        Color{77, 194, 103, 255},
        Color{127, 236, 124, 255},
        Color{235, 193, 99, 255},
    };
    for (const Chip& chip : _chips) {
        const float t = chip.age / chip.life;
        const float alpha = std::max(0.0f, 1.0f - t * t);
        const Vector2 p = game.worldToScreen({chip.pos.x, chip.pos.y});
        if (p.x < -10.0f || p.x > kScreenWidth + 10.0f || p.y < -10.0f || p.y > kScreenHeight + 10.0f) {
            continue;
        }
        const int size = t < 0.55f ? 3 : 2;
        DrawRectangle(static_cast<int>(std::round(p.x)) - size / 2,
                      static_cast<int>(std::round(p.y)) - size / 2,
                      size,
                      size,
                      fadeColor(chipColors[chip.seed % 3U], alpha));
    }

    for (const Item& cactus : _items) {
        if (!cactus.active) {
            continue;
        }
        const Vector2 p = game.worldToScreen({cactus.pos.x, cactus.pos.y});
        if (p.x < -28.0f || p.x > kScreenWidth + 28.0f) {
            continue;
        }

        const int ix = static_cast<int>(std::round(p.x));
        const int baseY =
            static_cast<int>(std::round(game.worldToScreen({cactus.pos.x, game._terrain.heightAt(cactus.pos.x)}).y));
        const int bend = static_cast<int>(std::round(cactus.wobble * 2.0f));
        const bool hitFlash = cactus.cooldown > 0.58f;
        if (textureLoaded(game._sprites.cactus)) {
            drawSpriteCentered(game._sprites.cactus,
                               {static_cast<float>(ix + bend), static_cast<float>(baseY - 13.0f)},
                               22.0f,
                               26.0f,
                               cactus.wobble * 4.6f);
            if (hitFlash) {
                drawPixelCactus(ix,
                                baseY,
                                bend,
                                Color{255, 236, 120, 230},
                                Color{230, 255, 190, 220},
                                Color{255, 255, 236, 215},
                                Color{255, 250, 205, 230});
            }
            continue;
        }

        drawPixelCactus(ix,
                        baseY,
                        bend,
                        hitFlash ? Color{255, 236, 120, 255} : Color{24, 88, 62, 255},
                        hitFlash ? Color{139, 255, 129, 255} : Color{50, 168, 86, 255},
                        Color{116, 230, 113, 255},
                        Color{239, 222, 151, 255});
    }
}

} // namespace ridge_dash

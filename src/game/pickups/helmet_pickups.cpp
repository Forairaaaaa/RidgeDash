/**
 * @file helmet_pickups.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-13
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

using namespace game_config;

// ── CRTP hook accessors ──────────────────────────────────────────────────

float HelmetPickups::pickupDistance() const
{
    return kHelmetPickupDistance;
}

// ── Lifecycle ────────────────────────────────────────────────────────────

void HelmetPickups::reset(RidgeDashGame& game)
{
    _items.clear();
    _items.reserve(8);

    std::uniform_real_distribution<float> gapDist(250.0f, 420.0f);
    _nextX = 60.0f + gapDist(game._rng) * 0.22f;
    stream(game, game._startX + kHelmetGenerateAhead);
}

// ── Streaming ────────────────────────────────────────────────────────────

void HelmetPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(250.0f, 420.0f);
    std::uniform_real_distribution<float> spawnChance(0.0f, 1.0f);
    while (_nextX < targetX) {
        const TerrainSample terrain = game._terrain.sampleAt(_nextX, 12.0f, game._rng);

        // Snow biome only.
        if (terrain.biome != TerrainBiome::Snow) {
            _nextX += 8.0f;
            continue;
        }
        // 50% chance to actually spawn — keep it rare.
        if (spawnChance(game._rng) > 0.5f) {
            _nextX += 6.0f;
            continue;
        }
        if (game._pickups.fuel().activeNear(_nextX, 6.5f) || game._pickups.coin().activeNear(_nextX, 5.5f) ||
            game._pickups.flea().activeInRange(_nextX - 5.8f, _nextX + 5.8f) ||
            game._pickups.rocket().activeNear(_nextX, 6.5f) || game._pickups.snowman().activeNear(_nextX, 6.0f) ||
            game._pickups.giantFlea().activeNear(_nextX, 6.5f)) {
            _nextX += 8.5f;
            continue;
        }

        doCreate(game, terrain);
        _nextX += gapDist(game._rng);
    }
}

// ── Creation / Collection (CRTP hooks) ───────────────────────────────────

void HelmetPickups::doCreate(RidgeDashGame& game, const TerrainSample& terrain)
{
    constexpr float kPi = 3.14159265358979323846f;
    std::uniform_real_distribution<float> phaseDist(0.0f, kPi * 2.0f);

    _items.push_back(Item{});
    Item& helmet = _items.back();
    // Float in the air like a rocket, not sitting on the ground.
    helmet.basePos = {terrain.x, terrain.y - 1.20f};
    helmet.pos = helmet.basePos;
    helmet.idlePhase = phaseDist(game._rng);

    createSensorBody(game, helmet.pos, kHelmetRadius, helmet.bodyId, helmet.shapeId);
}

bool HelmetPickups::doCollect(RidgeDashGame& game, Item& item)
{
    if (!item.active || !game.carValid()) {
        return false;
    }

    // Don't collect if already have a helmet.
    if (game._helmetActive) {
        return false;
    }

    item.active = false;
    if (b2Body_IsValid(item.bodyId)) {
        b2DestroyBody(item.bodyId);
    }
    item.bodyId = b2_nullBodyId;
    item.shapeId = b2_nullShapeId;

    game._helmetActive = true;
    game._pickups.effects().spawn({item.pos.x, item.pos.y}, PickupEffects::Kind::Helmet, game._runSeed);
    game.playSfx(AudioSystem::Sfx::Fuel); // Placeholder — replace with helmet SFX later.
    return true;
}

// ── Per-frame update ─────────────────────────────────────────────────────

void HelmetPickups::update(float dt)
{
    for (Item& helmet : _items) {
        if (!helmet.active) {
            continue;
        }
        helmet.idleTime += dt;
        helmet.pos = {helmet.basePos.x, helmet.basePos.y + std::sin(helmet.idleTime * 3.2f + helmet.idlePhase) * 0.18f};
        if (b2Body_IsValid(helmet.bodyId)) {
            b2Body_SetTransform(helmet.bodyId, {helmet.pos.x, helmet.pos.y}, b2MakeRot(0.0f));
        }
    }
}

// ── Drawing ──────────────────────────────────────────────────────────────

void HelmetPickups::draw(const RidgeDashGame& game) const
{
    for (const Item& item : _items) {
        if (!item.active) {
            continue;
        }
        const Vector2 p = game.worldToScreen({item.pos.x, item.pos.y});
        if (p.x < -20.0f || p.x > kScreenWidth + 20.0f) {
            continue;
        }

        const int ix = static_cast<int>(std::round(p.x));
        const int iy = static_cast<int>(std::round(p.y));
        if (textureLoaded(game._sprites.helmet)) {
            drawSpriteCentered(
                game._sprites.helmet, {static_cast<float>(ix), static_cast<float>(iy)}, 16.0f, 12.0f, 0.0f);
            continue;
        }

        // Fallback: simple helmet pixel art.
        DrawRectangle(ix - 5, iy - 5, 10, 3, Color{220, 220, 220, 255}); // top
        DrawRectangle(ix - 6, iy - 2, 12, 5, Color{180, 180, 180, 255}); // dome
        DrawRectangle(ix - 7, iy + 3, 14, 2, Color{140, 140, 140, 255}); // brim
        DrawRectangle(ix - 3, iy - 3, 2, 2, Color{255, 220, 80, 255});   // highlight
    }
}

} // namespace ridge_dash

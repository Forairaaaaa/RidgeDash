/**
 * @file magnet_pickups.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-16
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
#include <random>

namespace ridge_dash {

using namespace game_config;

// ── CRTP hook accessors ──────────────────────────────────────────────────

float MagnetPickups::pickupDistance() const
{
    return kMagnetPickupDistance;
}

// ── Lifecycle ────────────────────────────────────────────────────────────

void MagnetPickups::reset(RidgeDashGame& game)
{
    _items.clear();
    _items.reserve(8);
    _timer = 0.0f;

    std::uniform_real_distribution<float> gapDist(250.0f, 420.0f);
    _nextX = 60.0f + gapDist(game._rng) * 0.22f;
    stream(game, game._startX + kMagnetGenerateAhead);
}

// ── Streaming ────────────────────────────────────────────────────────────

void MagnetPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(250.0f, 420.0f);
    std::uniform_real_distribution<float> spawnChance(0.0f, 1.0f);
    while (_nextX < targetX) {
        const TerrainSample terrain = game._terrain.sampleAt(_nextX, 12.0f, game._rng);

        // Desert biome only.
        if (terrain.biome != TerrainBiome::Desert) {
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
            game._pickups.rocket().activeNear(_nextX, 6.5f) || game._pickups.cactus().activeNear(_nextX, 6.0f) ||
            game._pickups.snowman().activeNear(_nextX, 6.5f) || game._pickups.helmet().activeNear(_nextX, 6.5f) ||
            game._pickups.giantFlea().activeNear(_nextX, 6.5f)) {
            _nextX += 8.5f;
            continue;
        }

        doCreate(game, terrain);
        _nextX += gapDist(game._rng);
    }
}

// ── Creation / Collection (CRTP hooks) ───────────────────────────────────

void MagnetPickups::doCreate(RidgeDashGame& game, const TerrainSample& terrain)
{
    constexpr float kPi = 3.14159265358979323846f;
    std::uniform_real_distribution<float> phaseDist(0.0f, kPi * 2.0f);

    _items.push_back(Item{});
    Item& magnet = _items.back();
    // Float in the air like a helmet/rocket, not sitting on the ground.
    magnet.basePos = {terrain.x, terrain.y - 1.20f};
    magnet.pos = magnet.basePos;
    magnet.idlePhase = phaseDist(game._rng);

    createSensorBody(game, magnet.pos, kMagnetRadius, magnet.bodyId, magnet.shapeId);
}

bool MagnetPickups::doCollect(RidgeDashGame& game, Item& item)
{
    if (!item.active || !game.carValid()) {
        return false;
    }

    item.active = false;
    if (b2Body_IsValid(item.bodyId)) {
        b2DestroyBody(item.bodyId);
    }
    item.bodyId = b2_nullBodyId;
    item.shapeId = b2_nullShapeId;

    _timer = kMagnetDuration;
    game._pickups.effects().spawn({item.pos.x, item.pos.y}, PickupEffects::Kind::Magnet, game._runSeed);
    game.playSfx(AudioSystem::Sfx::Fuel); // Placeholder — replace with magnet SFX later.
    return true;
}

// ── Per-frame update ─────────────────────────────────────────────────────

void MagnetPickups::update(RidgeDashGame& game, float dt)
{
    // Decay the active timer.
    if (_timer > 0.0f) {
        _timer -= dt;
        if (_timer <= 0.0f) {
            _timer = 0.0f;
            // Finish burst at the car position, like rocket.
            if (game.carValid()) {
                const b2Vec2 carPos = game._vehicle.chassisPosition();
                game._pickups.effects().spawn({carPos.x, carPos.y}, PickupEffects::Kind::Magnet, game._runSeed);
            }
        }
    }

    // Idle animation for world items.
    for (Item& magnet : _items) {
        if (!magnet.active) {
            continue;
        }
        magnet.idleTime += dt;
        magnet.pos = {magnet.basePos.x, magnet.basePos.y + std::sin(magnet.idleTime * 3.2f + magnet.idlePhase) * 0.18f};
        if (b2Body_IsValid(magnet.bodyId)) {
            b2Body_SetTransform(magnet.bodyId, {magnet.pos.x, magnet.pos.y}, b2MakeRot(0.0f));
        }
    }
}

// ── Active query ──────────────────────────────────────────────────────────

bool MagnetPickups::active() const
{
    return _timer > 0.0f;
}

// ── Drawing ──────────────────────────────────────────────────────────────

void MagnetPickups::draw(const RidgeDashGame& game) const
{
    // --- Draw idle world items ---
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
        const float angle = std::sin(item.idleTime * 2.4f + item.idlePhase) * 5.0f;
        if (textureLoaded(game._sprites.magnet)) {
            drawSpriteCentered(
                game._sprites.magnet, {static_cast<float>(ix), static_cast<float>(iy)}, 33.0f, 14.0f, angle);
            continue;
        }

        // Fallback: U-shape magnet pixel art (33:14 ratio, wider than tall).
        DrawRectangle(ix - 16, iy - 4, 32, 4, Color{220, 220, 80, 255}); // top bar
        DrawRectangle(ix - 16, iy, 4, 10, Color{220, 220, 80, 255});     // left leg
        DrawRectangle(ix + 12, iy, 4, 10, Color{220, 220, 80, 255});     // right leg
        DrawRectangle(ix - 4, iy, 8, 3, Color{180, 80, 80, 255});        // red center
    }
}

// ── Cleanup ────────────────────────────────────────────────────────────────

void MagnetPickups::doClear()
{
    _timer = 0.0f;
}

} // namespace ridge_dash

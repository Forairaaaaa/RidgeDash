/**
 * @file snowman_pickups.cpp
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

bool nearPoint(Vector2 a, Vector2 b, float distance)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy <= distance * distance;
}

} // namespace

using namespace game_config;

void SnowmanPickups::clear()
{
    _items.clear();
    _nextX = 0.0f;
    _boostTimer = 0.0f;
    _activeBoost = 1.0f;
}

void SnowmanPickups::reset(RidgeDashGame& game)
{
    clear();
    _items.reserve(12);

    std::uniform_real_distribution<float> gapDist(36.0f, 68.0f);
    _nextX = 86.0f + gapDist(game._rng) * 0.55f;
    stream(game, game._startX + kSnowmanGenerateAhead);
}

void SnowmanPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(36.0f, 68.0f);
    while (_nextX < targetX) {
        const TerrainSample terrain = game._terrain.sampleAt(_nextX, 12.0f, game._rng);

        if (terrain.biome != TerrainBiome::Snow) {
            _nextX += 8.0f;
            continue;
        }
        if (game._pickups.fuel().activeNear(_nextX, 5.2f) || game._pickups.coin().activeNear(_nextX, 4.4f) ||
            game._pickups.flea().activeInRange(_nextX - 5.0f, _nextX + 5.0f) ||
            game._pickups.rocket().activeNear(_nextX, 6.0f) || game._pickups.cactus().activeNear(_nextX, 5.6f) ||
            game._pickups.helmet().activeNear(_nextX, 6.0f)) {
            _nextX += 6.4f;
            continue;
        }

        create(game, terrain);
        _nextX += gapDist(game._rng);
    }
}

void SnowmanPickups::create(RidgeDashGame& game, const TerrainSample& terrain)
{
    if (!b2World_IsValid(game._worldId)) {
        return;
    }

    std::uniform_real_distribution<float> boostDist(0.85f, 1.30f);

    _items.push_back(Item{});
    Item& snowman = _items.back();
    snowman.pos = {terrain.x, terrain.y - 0.78f};
    snowman.boost = boostDist(game._rng);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {snowman.pos.x, snowman.pos.y};
    snowman.bodyId = b2CreateBody(game._worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;
    b2Circle circle = {{0.0f, 0.0f}, kSnowmanRadius};
    snowman.shapeId = b2CreateCircleShape(snowman.bodyId, &shapeDef, &circle);
}

void SnowmanPickups::trim(float minX)
{
    auto it = _items.begin();
    while (it != _items.end()) {
        if (it->active && it->pos.x >= minX) {
            ++it;
            continue;
        }
        if (b2Body_IsValid(it->bodyId)) {
            b2DestroyBody(it->bodyId);
        }
        it = _items.erase(it);
    }
}

void SnowmanPickups::applyBoost(RidgeDashGame& game)
{
    if (!game.carValid()) {
        return;
    }

    const b2Vec2 velocity = game._vehicle.chassisVelocity();
    const float throttle = clampf(_boostTimer / 1.05f, 0.0f, 1.0f);
    // Randomised boost (0.85–1.30): ~1/2 average force of the old fixed 18+8×throttle,
    // max below 3/4 of the old peak (19.5 vs 26).
    const float forceX = _activeBoost * (8.0f + throttle * 7.0f) + std::max(0.0f, velocity.x) * 0.12f;
    game._vehicle.applyMainBodyForcePerMass({forceX, -0.8f}, 1.0f, 0.55f, 0.55f);
    game._vehicle.applyChassisTorque(-0.4f * throttle * _activeBoost);
}

bool SnowmanPickups::collect(RidgeDashGame& game, Item& snowman)
{
    if (!snowman.active || !game.carValid()) {
        return false;
    }

    const b2Vec2 velocity = game._vehicle.chassisVelocity();
    _activeBoost = snowman.boost;
    game._vehicle.applyChassisDeltaVelocity({3.0f + std::max(0.0f, velocity.x) * 0.06f, -0.42f});
    _boostTimer = 1.05f;

    snowman.active = false;
    game._pickups.effects().spawn({snowman.pos.x, snowman.pos.y - 0.12f}, PickupEffects::Kind::Snowman, game._runSeed);
    game.playSfx(AudioSystem::Sfx::Snowman);
    if (b2Body_IsValid(snowman.bodyId)) {
        b2DestroyBody(snowman.bodyId);
    }
    snowman.bodyId = b2_nullBodyId;
    snowman.shapeId = b2_nullShapeId;
    return true;
}

void SnowmanPickups::update(RidgeDashGame& game, float dt)
{
    (void)game;
    // Timer counts down in real time (per frame); the boost force is applied in
    // applyStepForces once per physics step so it stays framerate-independent.
    _boostTimer = std::max(0.0f, _boostTimer - dt);
}

void SnowmanPickups::applyStepForces(RidgeDashGame& game)
{
    if (_boostTimer <= 0.0f) {
        return;
    }
    applyBoost(game);
}

bool SnowmanPickups::collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape)
{
    if (!game._vehicle.shapeBelongsToVehicle(otherShape)) {
        return false;
    }
    for (Item& snowman : _items) {
        if (snowman.active && b2Shape_IsValid(snowman.shapeId) && B2_ID_EQUALS(snowman.shapeId, pickupShape)) {
            return collect(game, snowman);
        }
    }
    return false;
}

bool SnowmanPickups::collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus)
{
    bool collected = false;
    for (Item& snowman : _items) {
        if (!snowman.active) {
            continue;
        }
        for (int i = 0; i < count; ++i) {
            if (nearPoint(points[i], snowman.pos, kSnowmanPickupDistance + speedBonus)) {
                collected = collect(game, snowman) || collected;
                break;
            }
        }
    }
    return collected;
}

bool SnowmanPickups::activeNear(float x, float distance) const
{
    for (const Item& snowman : _items) {
        if (snowman.active && std::abs(snowman.pos.x - x) <= distance) {
            return true;
        }
    }
    return false;
}

void SnowmanPickups::draw(const RidgeDashGame& game) const
{
    for (const Item& snowman : _items) {
        if (!snowman.active) {
            continue;
        }
        const Vector2 p = game.worldToScreen({snowman.pos.x, snowman.pos.y});
        if (p.x < -24.0f || p.x > kScreenWidth + 24.0f) {
            continue;
        }

        const int ix = static_cast<int>(std::round(p.x));
        const int baseY =
            static_cast<int>(std::round(game.worldToScreen({snowman.pos.x, game._terrain.heightAt(snowman.pos.x)}).y));
        if (textureLoaded(game._sprites.snowman)) {
            drawSpriteCentered(
                game._sprites.snowman, {static_cast<float>(ix), static_cast<float>(baseY - 11.0f)}, 26.0f, 22.0f, 0.0f);
            continue;
        }

        DrawRectangle(ix - 7, baseY - 13, 14, 9, Color{236, 250, 255, 255});
        DrawRectangle(ix - 5, baseY - 20, 10, 8, Color{246, 254, 255, 255});
        DrawRectangle(ix - 4, baseY - 18, 2, 2, Color{25, 32, 38, 255});
        DrawRectangle(ix + 3, baseY - 18, 2, 2, Color{25, 32, 38, 255});
        DrawRectangle(ix, baseY - 16, 4, 1, Color{240, 114, 65, 255});
        DrawRectangle(ix - 8, baseY - 4, 16, 3, Color{179, 210, 225, 255});
    }
}

void SnowmanPickups::forceSpawnAt(RidgeDashGame& game, float x)
{
    const TerrainSample terrain = game._terrain.sampleAt(x, 12.0f, game._rng);
    create(game, terrain);
}

} // namespace ridge_dash

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
namespace {

constexpr float kPi = 3.14159265358979323846f;

bool nearPoint(Vector2 a, Vector2 b, float distance)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy <= distance * distance;
}

} // namespace

using namespace game_config;

void HelmetPickups::clear()
{
    _items.clear();
    _nextX = 0.0f;
}

void HelmetPickups::reset(RidgeDashGame& game)
{
    _items.clear();
    _items.reserve(8);

    std::uniform_real_distribution<float> gapDist(250.0f, 420.0f);
    _nextX = 60.0f + gapDist(game._rng) * 0.22f;
    stream(game, game._startX + kHelmetGenerateAhead);
}

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
            game._pickups.rocket().activeNear(_nextX, 6.5f) ||
            game._pickups.snowman().activeNear(_nextX, 6.0f) ||
            game._pickups.giantFlea().activeNear(_nextX, 6.5f)) {
            _nextX += 8.5f;
            continue;
        }

        create(game, terrain);
        _nextX += gapDist(game._rng);
    }
}

void HelmetPickups::create(RidgeDashGame& game, const TerrainSample& terrain)
{
    if (!b2World_IsValid(game._worldId)) {
        return;
    }

    std::uniform_real_distribution<float> phaseDist(0.0f, kPi * 2.0f);

    _items.push_back(Item{});
    Item& helmet = _items.back();
    // Float in the air like a rocket, not sitting on the ground.
    helmet.basePos = {terrain.x, terrain.y - 1.20f};
    helmet.pos = helmet.basePos;
    helmet.idlePhase = phaseDist(game._rng);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {helmet.pos.x, helmet.pos.y};
    helmet.bodyId = b2CreateBody(game._worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;
    b2Circle circle = {{0.0f, 0.0f}, kHelmetRadius};
    helmet.shapeId = b2CreateCircleShape(helmet.bodyId, &shapeDef, &circle);
}

void HelmetPickups::trim(float minX)
{
    auto it = _items.begin();
    while (it != _items.end()) {
        if (it->active && it->basePos.x >= minX) {
            ++it;
            continue;
        }
        if (b2Body_IsValid(it->bodyId)) {
            b2DestroyBody(it->bodyId);
        }
        it = _items.erase(it);
    }
}

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

bool HelmetPickups::collect(RidgeDashGame& game, Item& item)
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

bool HelmetPickups::collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape)
{
    if (!game._vehicle.shapeBelongsToVehicle(otherShape)) {
        return false;
    }
    for (Item& item : _items) {
        if (item.active && b2Shape_IsValid(item.shapeId) && B2_ID_EQUALS(item.shapeId, pickupShape)) {
            return collect(game, item);
        }
    }
    return false;
}

bool HelmetPickups::collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus)
{
    bool collected = false;
    for (Item& item : _items) {
        if (!item.active) {
            continue;
        }
        for (int i = 0; i < count; ++i) {
            if (nearPoint(points[i], item.pos, kHelmetPickupDistance + speedBonus)) {
                collected = collect(game, item) || collected;
                break;
            }
        }
    }
    return collected;
}

bool HelmetPickups::activeNear(float x, float distance) const
{
    for (const Item& item : _items) {
        if (item.active && std::abs(item.basePos.x - x) <= distance) {
            return true;
        }
    }
    return false;
}

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
        DrawRectangle(ix - 5, iy - 5, 10, 3, Color{220, 220, 220, 255});   // top
        DrawRectangle(ix - 6, iy - 2, 12, 5, Color{180, 180, 180, 255});    // dome
        DrawRectangle(ix - 7, iy + 3, 14, 2, Color{140, 140, 140, 255});    // brim
        DrawRectangle(ix - 3, iy - 3, 2, 2, Color{255, 220, 80, 255});      // highlight
    }
}

} // namespace ridge_dash

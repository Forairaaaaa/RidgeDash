/**
 * @file fuel_pickups.cpp
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

void FuelPickups::clear()
{
    _items.clear();
    _nextX = 0.0f;
}

void FuelPickups::reset(RidgeDashGame& game)
{
    _items.clear();
    _items.reserve(16);

    std::uniform_real_distribution<float> gapDist(40.0f, 72.0f);
    _nextX = 28.0f + gapDist(game._rng) * 0.45f;
    stream(game, game._startX + kFuelGenerateAhead);
}

void FuelPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(40.0f, 72.0f);
    while (_nextX < targetX) {
        const TerrainSample terrain = game._terrain.sampleAt(_nextX, 8.0f, game._rng);
        create(game, terrain);
        _nextX += gapDist(game._rng);
    }
}

void FuelPickups::create(RidgeDashGame& game, const TerrainSample& terrain)
{
    if (!b2World_IsValid(game._worldId)) {
        return;
    }

    _items.push_back(Item{});
    Item& fuel = _items.back();
    fuel.pos = {terrain.x, terrain.y - 1.25f};

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {fuel.pos.x, fuel.pos.y};
    fuel.bodyId = b2CreateBody(game._worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;
    b2Circle circle = {{0.0f, 0.0f}, kFuelCanRadius};
    fuel.shapeId = b2CreateCircleShape(fuel.bodyId, &shapeDef, &circle);
}

void FuelPickups::trim(float minX)
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

bool FuelPickups::collect(RidgeDashGame& game, Item& fuel)
{
    if (!fuel.active) {
        return false;
    }

    game._pickups.effects().spawn(fuel.pos, PickupEffects::Kind::Fuel, game._runSeed);
    fuel.active = false;
    game._runController.refillFuel();
    if (b2Body_IsValid(fuel.bodyId)) {
        b2DestroyBody(fuel.bodyId);
    }
    fuel.bodyId = b2_nullBodyId;
    fuel.shapeId = b2_nullShapeId;
    return true;
}

bool FuelPickups::collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape)
{
    if (!game._vehicle.shapeBelongsToVehicle(otherShape)) {
        return false;
    }
    for (Item& fuel : _items) {
        if (fuel.active && b2Shape_IsValid(fuel.shapeId) && B2_ID_EQUALS(fuel.shapeId, pickupShape)) {
            return collect(game, fuel);
        }
    }
    return false;
}

bool FuelPickups::collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus)
{
    bool collected = false;
    for (Item& fuel : _items) {
        if (!fuel.active) {
            continue;
        }
        for (int i = 0; i < count; ++i) {
            if (nearPoint(points[i], fuel.pos, kFuelPickupDistance + speedBonus)) {
                collected = collect(game, fuel) || collected;
                break;
            }
        }
    }
    return collected;
}

bool FuelPickups::activeInRange(float minX, float maxX) const
{
    for (const Item& fuel : _items) {
        if (fuel.active && fuel.pos.x >= minX && fuel.pos.x <= maxX) {
            return true;
        }
    }
    return false;
}

bool FuelPickups::activeNear(float x, float distance) const
{
    for (const Item& fuel : _items) {
        if (fuel.active && std::abs(fuel.pos.x - x) <= distance) {
            return true;
        }
    }
    return false;
}

void FuelPickups::draw(const RidgeDashGame& game) const
{
    for (const Item& fuel : _items) {
        if (!fuel.active) {
            continue;
        }
        const Vector2 p = game.worldToScreen({fuel.pos.x, fuel.pos.y});
        if (p.x < -20.0f || p.x > kScreenWidth + 20.0f) {
            continue;
        }
        const int ix = static_cast<int>(std::round(p.x));
        const int iy = static_cast<int>(std::round(p.y));
        if (textureLoaded(game._sprites.fuelCan)) {
            drawSpriteCentered(
                game._sprites.fuelCan, {static_cast<float>(ix), static_cast<float>(iy)}, 15.0f, 19.0f, 0.0f);
        } else {
            DrawRectangle(ix - 5, iy - 7, 10, 14, Color{196, 48, 58, 255});
            DrawRectangle(ix - 3, iy - 10, 6, 4, Color{242, 202, 95, 255});
            DrawRectangle(ix + 2, iy - 4, 3, 7, Color{241, 100, 70, 255});
            DrawRectangleLines(ix - 5, iy - 7, 10, 14, Color{82, 32, 39, 255});
        }
    }
}

} // namespace ridge_dash

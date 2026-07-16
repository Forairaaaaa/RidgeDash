/**
 * @file pickup_base_impl.hpp
 * @brief Out-of-line template definitions for PickupCollection.
 *
 * Must be included AFTER ridge_dash_game.hpp (which provides the full
 * definitions of RidgeDashGame and TerrainSample).

 * @version 0.1
 * @date 2026-07-16
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "game/pickups/pickup_base.hpp"
#include "game/ridge_dash_game.hpp"

namespace ridge_dash {

// ── Out-of-line template definitions ────────────────────────────────────

template <typename Derived>
bool PickupCollection<Derived>::collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape)
{
    if (!game._vehicle.shapeBelongsToVehicle(otherShape)) {
        return false;
    }
    for (auto& item : derived().items()) {
        if (item.active && b2Shape_IsValid(item.shapeId) && B2_ID_EQUALS(item.shapeId, pickupShape)) {
            return derived().doCollect(game, item);
        }
    }
    return false;
}

template <typename Derived>
void PickupCollection<Derived>::forceSpawnAt(RidgeDashGame& game, float x)
{
    const TerrainSample terrain = game._terrain.sampleAt(x, 12.0f, game._rng);
    derived().doCreate(game, terrain);
}

template <typename Derived>
void PickupCollection<Derived>::createSensorBody(RidgeDashGame& game,
                                                 Vector2 pos,
                                                 float radius,
                                                 b2BodyId& outBody,
                                                 b2ShapeId& outShape)
{
    if (!b2World_IsValid(game._worldId)) {
        return;
    }

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {pos.x, pos.y};
    outBody = b2CreateBody(game._worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;
    b2Circle circle = {{0.0f, 0.0f}, radius};
    outShape = b2CreateCircleShape(outBody, &shapeDef, &circle);
}

} // namespace ridge_dash

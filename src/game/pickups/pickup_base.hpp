/**
 * @file pickup_base.hpp
 * @brief CRTP base template for pickup collections — consolidates duplicated
 *        clear/trim/collectByShape/collectOverlaps/activeNear/forceSpawnAt
 *        logic shared across all standard pickup types.
 *
 * Each derived class must publicly provide:
 *   - A nested `Item` struct (with bodyId, shapeId, pos/active at minimum)
 *   - Vector2 itemPos(const Item&) const
 *   - float   itemTrimX(const Item&) const
 *   - float   pickupDistance() const
 *   - void    doCreate(RidgeDashGame&, const TerrainSample&)
 *   - bool    doCollect(RidgeDashGame&, Item&)
 *   - std::vector<Item>& items()
 *   - const std::vector<Item>& items() const
 *   - float& nextX()
 *   - const float& nextX() const
 *
 * Optional overrides (no-op by default):
 *   - void doClear()
 *   - void doTrimExtra(float minX)
 *
 * @version 0.1
 * @date 2026-07-16
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "platform/raylib_compat.hpp"

#include <box2d/box2d.h>
#include <cmath>
#include <vector>

namespace ridge_dash {

class RidgeDashGame;
struct TerrainSample;

namespace pickup_detail {

inline bool nearPoint(Vector2 a, Vector2 b, float distance)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy <= distance * distance;
}

} // namespace pickup_detail

template <typename Derived>
class PickupCollection {
public:
    // ── Consolidated boilerplate (implemented once) ──────────────────────

    void clear()
    {
        derived().items().clear();
        derived().nextX() = 0.0f;
        derived().doClear();
    }

    void trim(float minX)
    {
        auto& v = derived().items();
        auto it = v.begin();
        while (it != v.end()) {
            if (it->active && derived().itemTrimX(*it) >= minX) {
                ++it;
                continue;
            }
            if (b2Body_IsValid(it->bodyId)) {
                b2DestroyBody(it->bodyId);
            }
            it = v.erase(it);
        }
        derived().doTrimExtra(minX);
    }

    bool collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape);

    bool collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus)
    {
        bool collected = false;
        for (auto& item : derived().items()) {
            if (!item.active) {
                continue;
            }
            for (int i = 0; i < count; ++i) {
                if (pickup_detail::nearPoint(
                        points[i], derived().itemPos(item), derived().pickupDistance() + speedBonus)) {
                    collected = derived().doCollect(game, item) || collected;
                    break;
                }
            }
        }
        return collected;
    }

    bool activeNear(float x, float distance) const
    {
        for (const auto& item : derived().items()) {
            if (item.active && std::abs(derived().itemTrimX(item) - x) <= distance) {
                return true;
            }
        }
        return false;
    }

    bool activeInRange(float minX, float maxX) const
    {
        for (const auto& item : derived().items()) {
            if (item.active && derived().itemTrimX(item) >= minX && derived().itemTrimX(item) <= maxX) {
                return true;
            }
        }
        return false;
    }

    void forceSpawnAt(RidgeDashGame& game, float x);

    // ── Helper for derived classes ───────────────────────────────────────

    static void createSensorBody(RidgeDashGame& game,
                                 Vector2 pos,
                                 float radius,
                                 b2BodyId& outBody,
                                 b2ShapeId& outShape);

protected:
    const Derived& derived() const
    {
        return static_cast<const Derived&>(*this);
    }
    Derived& derived()
    {
        return static_cast<Derived&>(*this);
    }

    // Default no-op hooks — derived classes override as needed.
    void doClear() {}
    void doTrimExtra(float /*minX*/) {}
};

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

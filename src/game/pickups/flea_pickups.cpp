/**
 * @file flea_pickups.cpp
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

float fleaSide(Vector2 basePos, float boost)
{
    return ((static_cast<int>(basePos.x * 13.0f + boost * 19.0f) & 1) == 0) ? -1.0f : 1.0f;
}

} // namespace

using namespace game_config;

void FleaPickups::clear()
{
    _items.clear();
    _nextX = 0.0f;
}

void FleaPickups::reset(RidgeDashGame& game)
{
    _items.clear();
    _items.reserve(12);

    std::uniform_real_distribution<float> gapDist(150.0f, 240.0f);
    _nextX = 34.0f + gapDist(game._rng) * 0.30f;
    stream(game, game._startX + kFleaGenerateAhead);
}

void FleaPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(150.0f, 240.0f);
    while (_nextX < targetX) {
        const TerrainSample terrain = game._terrain.sampleAt(_nextX, 10.0f, game._rng);

        if (terrain.biome != TerrainBiome::Mountain) {
            _nextX += 8.0f;
            continue;
        }
        if (game._pickups.fuel().activeNear(_nextX, 5.0f)) {
            _nextX += 6.8f;
            continue;
        }
        if (game._pickups.coin().activeNear(_nextX, 3.8f)) {
            _nextX += 4.8f;
            continue;
        }
        if (game._pickups.rocket().activeNear(_nextX, 5.8f)) {
            _nextX += 6.2f;
            continue;
        }
        if (game._pickups.cactus().activeNear(_nextX, 5.2f)) {
            _nextX += 6.0f;
            continue;
        }

        create(game, terrain);
        _nextX += gapDist(game._rng);
    }
}

void FleaPickups::create(RidgeDashGame& game, const TerrainSample& terrain)
{
    if (!b2World_IsValid(game._worldId)) {
        return;
    }

    std::uniform_real_distribution<float> boostDist(0.96f, 1.42f);
    std::uniform_real_distribution<float> idleDist(0.20f, 1.45f);

    _items.push_back(Item{});
    Item& flea = _items.back();
    flea.basePos = {terrain.x, terrain.y - 0.74f};
    flea.pos = flea.basePos;
    flea.boost = boostDist(game._rng);
    flea.idleCooldown = idleDist(game._rng);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {flea.pos.x, flea.pos.y};
    flea.bodyId = b2CreateBody(game._worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;
    b2Circle circle = {{0.0f, 0.0f}, kFleaRadius};
    flea.shapeId = b2CreateCircleShape(flea.bodyId, &shapeDef, &circle);
}

void FleaPickups::trim(float minX)
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

void FleaPickups::applyBoost(RidgeDashGame& game, float boost)
{
    if (!game.carValid()) {
        return;
    }

    const b2Vec2 velocity = game._vehicle.chassisVelocity();
    const float upwardDelta =
        9.4f + boost * 3.7f + std::max(0.0f, velocity.y) * 0.34f - std::max(0.0f, -velocity.y) * 0.10f;
    const b2Vec2 deltaVelocity = {clampf(velocity.x * 0.22f, -2.8f, 5.8f), -clampf(upwardDelta, 8.4f, 13.4f)};

    game._vehicle.applyMainBodyDeltaVelocity(deltaVelocity, 1.0f, 1.0f, 1.0f);
    game._vehicle.applyChassisAngularImpulse((velocity.x >= 0.0f ? -1.0f : 1.0f) * (0.22f + boost * 0.08f));
}

bool FleaPickups::collect(RidgeDashGame& game, Item& flea)
{
    if (!flea.active || flea.cooldown > 0.0f || flea.triggeredJump) {
        return false;
    }

    applyBoost(game, flea.boost);
    const float jumpHeight = 0.72f + flea.boost * 0.74f;
    flea.jumpOffset = 0.0f;
    flea.jumpVelocity = -std::sqrt(2.0f * 24.0f * jumpHeight);
    flea.idleCooldown = 0.0f;
    flea.triggeredJump = true;
    flea.tiltVelocity += fleaSide(flea.basePos, flea.boost) * (18.0f + flea.boost * 10.0f);
    return true;
}

void FleaPickups::update(RidgeDashGame& game, float dt)
{
    constexpr float kFleaGravity = 24.0f;
    constexpr float kTriggeredRestitution = 0.42f;
    constexpr float kIdleRestitution = 0.34f;

    for (Item& flea : _items) {
        if (!flea.active) {
            continue;
        }

        if (flea.cooldown > 0.0f && !flea.triggeredJump && flea.jumpOffset == 0.0f && flea.jumpVelocity == 0.0f) {
            flea.cooldown = std::max(0.0f, flea.cooldown - dt);
        }

        if (flea.jumpOffset < 0.0f || flea.jumpVelocity != 0.0f) {
            flea.jumpVelocity += kFleaGravity * dt;
            flea.jumpOffset += flea.jumpVelocity * dt;
            if (flea.jumpOffset > 0.0f) {
                flea.jumpOffset = 0.0f;
                flea.tiltVelocity +=
                    fleaSide(flea.basePos, flea.boost) * clampf(std::abs(flea.jumpVelocity) * 4.2f, 8.0f, 34.0f);
                const float minBounce = flea.triggeredJump ? 2.4f : 1.7f;
                const float restitution = flea.triggeredJump ? kTriggeredRestitution : kIdleRestitution;
                if (std::abs(flea.jumpVelocity) > minBounce) {
                    flea.jumpVelocity = -flea.jumpVelocity * restitution;
                } else {
                    flea.jumpVelocity = 0.0f;
                    if (flea.triggeredJump) {
                        flea.cooldown = 0.42f + flea.boost * 0.16f;
                        flea.idleCooldown = 0.34f + flea.boost * 0.10f;
                    } else {
                        flea.idleCooldown = 0.62f + flea.boost * 0.28f;
                    }
                    flea.triggeredJump = false;
                }
            }
        } else if (flea.cooldown <= 0.0f) {
            flea.idleCooldown = std::max(0.0f, flea.idleCooldown - dt);
            if (flea.idleCooldown == 0.0f) {
                const float jumpHeight = 0.22f + flea.boost * 0.34f;
                flea.jumpVelocity = -std::sqrt(2.0f * kFleaGravity * jumpHeight);
                flea.tiltVelocity += -fleaSide(flea.basePos, flea.boost) * (4.0f + flea.boost * 2.6f);
            }
        }

        flea.tiltVelocity += -flea.tilt * 34.0f * dt;
        flea.tiltVelocity *= std::pow(0.035f, dt);
        flea.tilt += flea.tiltVelocity * dt;
        flea.tilt = clampf(flea.tilt, -14.0f, 14.0f);
        flea.pos = {flea.basePos.x, flea.basePos.y + flea.jumpOffset};
        if (b2Body_IsValid(flea.bodyId)) {
            b2Body_SetTransform(flea.bodyId, {flea.pos.x, flea.pos.y}, b2MakeRot(0.0f));
        }
    }
}

bool FleaPickups::collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape)
{
    if (!game._vehicle.shapeBelongsToVehicle(otherShape)) {
        return false;
    }
    for (Item& flea : _items) {
        if (flea.active && b2Shape_IsValid(flea.shapeId) && B2_ID_EQUALS(flea.shapeId, pickupShape)) {
            return collect(game, flea);
        }
    }
    return false;
}

bool FleaPickups::collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus)
{
    bool collected = false;
    for (Item& flea : _items) {
        if (!flea.active) {
            continue;
        }
        for (int i = 0; i < count; ++i) {
            if (nearPoint(points[i], flea.pos, kFleaPickupDistance + speedBonus)) {
                collected = collect(game, flea) || collected;
                break;
            }
        }
    }
    return collected;
}

bool FleaPickups::activeInRange(float minX, float maxX) const
{
    for (const Item& flea : _items) {
        if (flea.active && flea.basePos.x >= minX && flea.basePos.x <= maxX) {
            return true;
        }
    }
    return false;
}

void FleaPickups::draw(const RidgeDashGame& game) const
{
    for (const Item& flea : _items) {
        if (!flea.active) {
            continue;
        }
        const Vector2 p = game.worldToScreen({flea.pos.x, flea.pos.y});
        if (p.x < -18.0f || p.x > kScreenWidth + 18.0f) {
            continue;
        }

        const int ix = static_cast<int>(std::round(p.x));
        const int iy = static_cast<int>(std::round(p.y));
        if (textureLoaded(game._sprites.flea)) {
            drawSpriteCentered(
                game._sprites.flea, {static_cast<float>(ix), static_cast<float>(iy + 3)}, 10.0f, 12.0f, flea.tilt);
            continue;
        }

        const bool grounded = flea.jumpOffset == 0.0f && flea.jumpVelocity >= 0.0f;
        const int squash = grounded && flea.cooldown > 0.0f ? 1 : 0;
        const Color body = Color{94, 239, 189, 255};
        const Color belly = Color{195, 255, 230, 255};
        const Color dark = Color{34, 95, 86, 255};
        const Color eye = Color{30, 36, 46, 255};

        DrawRectangle(ix - 5, iy - 4 + squash, 10, 7 - squash, body);
        DrawRectangle(ix - 3, iy - 6 + squash, 6, 3, belly);
        DrawRectangle(ix - 6, iy - 2, 2, 3, dark);
        DrawRectangle(ix + 4, iy - 2, 2, 3, dark);
        DrawRectangle(ix - 2, iy - 4 + squash, 1, 1, eye);
        DrawRectangle(ix + 2, iy - 4 + squash, 1, 1, eye);
        DrawRectangle(ix - 7, iy + 3, 4, 2, dark);
        DrawRectangle(ix + 3, iy + 3, 4, 2, dark);
        DrawRectangle(ix - 5, iy + 5, 3, 1, dark);
        DrawRectangle(ix + 2, iy + 5, 3, 1, dark);
        if (flea.jumpOffset < -0.08f) {
            DrawRectangle(ix - 4, iy + 6, 2, 2, fadeColor(Color{210, 255, 239, 255}, 0.55f));
            DrawRectangle(ix + 2, iy + 6, 2, 2, fadeColor(Color{210, 255, 239, 255}, 0.55f));
        }
    }
}

} // namespace ridge_dash

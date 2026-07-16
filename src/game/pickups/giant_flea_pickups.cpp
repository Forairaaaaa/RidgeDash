/**
 * @file giant_flea_pickups.cpp
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

float giantFleaSide(Vector2 basePos, float boost)
{
    return ((static_cast<int>(basePos.x * 13.0f + boost * 19.0f) & 1) == 0) ? -1.0f : 1.0f;
}

} // namespace

using namespace game_config;

void GiantFleaPickups::reset(RidgeDashGame& game)
{
    _items.clear();
    _items.reserve(8);
    _attached = false;
    _bouncesRemaining = 0;
    _wasGrounded = false;
    _activeBoost = 1.0f;

    std::uniform_real_distribution<float> gapDist(200.0f, 350.0f);
    _nextX = 48.0f + gapDist(game._rng) * 0.16f;
    stream(game, game._startX + kGiantFleaGenerateAhead);
}

void GiantFleaPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(200.0f, 350.0f);
    while (_nextX < targetX) {
        const TerrainSample terrain = game._terrain.sampleAt(_nextX, 12.0f, game._rng);

        // Stone biome only.
        if (terrain.biome != TerrainBiome::Stone) {
            _nextX += 8.0f;
            continue;
        }
        if (game._pickups.fuel().activeNear(_nextX, 6.5f) || game._pickups.coin().activeNear(_nextX, 5.5f) ||
            game._pickups.flea().activeInRange(_nextX - 5.8f, _nextX + 5.8f) ||
            game._pickups.rocket().activeNear(_nextX, 6.5f) || game._pickups.snowman().activeNear(_nextX, 6.0f) ||
            game._pickups.helmet().activeNear(_nextX, 6.5f)) {
            _nextX += 8.5f;
            continue;
        }

        doCreate(game, terrain);
        _nextX += gapDist(game._rng);
    }
}

void GiantFleaPickups::doCreate(RidgeDashGame& game, const TerrainSample& terrain)
{
    std::uniform_real_distribution<float> boostDist(0.96f, 1.42f);
    std::uniform_real_distribution<float> idleDist(0.20f, 1.45f);

    _items.push_back(Item{});
    Item& gf = _items.back();
    gf.basePos = {terrain.x, terrain.y - 0.90f};
    gf.pos = gf.basePos;
    gf.boost = boostDist(game._rng);
    gf.idleCooldown = idleDist(game._rng);

    createSensorBody(game, gf.pos, kGiantFleaRadius, gf.bodyId, gf.shapeId);
}

void GiantFleaPickups::applyBounce(RidgeDashGame& game)
{
    if (!game.carValid()) {
        return;
    }

    const b2Vec2 velocity = game._vehicle.chassisVelocity();
    // Same formula as regular flea: boost (0.96-1.42) x 3.7 randomizes the upward push.
    const float upwardDelta =
        9.6f + _activeBoost * 3.7f + std::max(0.0f, velocity.y) * 0.34f - std::max(0.0f, -velocity.y) * 0.10f;
    const b2Vec2 deltaVelocity = {clampf(velocity.x * 0.22f, -2.8f, 5.8f), -clampf(upwardDelta, 9.5f, 15.0f)};

    game._vehicle.applyMainBodyDeltaVelocity(deltaVelocity, 1.0f, 1.0f, 1.0f);
    game._vehicle.applyChassisAngularImpulse((velocity.x >= 0.0f ? -1.0f : 1.0f) * (0.22f + _activeBoost * 0.08f));

    // Spawn dust effect at the vehicle's wheels (large range, like flea but bigger).
    const b2Vec2 rearWheel = game._vehicle.rearWheelPosition();
    game._pickups.effects().spawn({rearWheel.x, rearWheel.y + 0.10f}, PickupEffects::Kind::GiantFlea, game._runSeed);
    game.playSfx(AudioSystem::Sfx::FleaJump);
}

bool GiantFleaPickups::doCollect(RidgeDashGame& game, Item& item)
{
    if (!item.active || item.cooldown > 0.0f || item.triggeredJump || _attached || !game.carValid()) {
        return false;
    }

    // Mark the world item as collected (inactive).
    item.active = false;
    if (b2Body_IsValid(item.bodyId)) {
        b2DestroyBody(item.bodyId);
    }
    item.bodyId = b2_nullBodyId;
    item.shapeId = b2_nullShapeId;

    // First bounce: immediate boost like a flea.
    _activeBoost = item.boost;
    applyBounce(game);
    game._pickups.effects().spawn({item.pos.x, item.pos.y + 0.14f}, PickupEffects::Kind::GiantFlea, game._runSeed);

    // Attach: 2 more bounces on subsequent landings (3 total including this one).
    // Set _wasGrounded = true so the next frame's update() doesn't trigger a second
    // bounce before physics has processed the velocity from this first one.
    _attached = true;
    _bouncesRemaining = 2;
    _wasGrounded = true;
    return true;
}

void GiantFleaPickups::update(RidgeDashGame& game, float dt)
{
    // --- Terrain items: idle hopping (same physics as regular flea but bigger) ---
    constexpr float kGiantFleaGravity = 24.0f;
    constexpr float kTriggeredRestitution = 0.42f;
    constexpr float kIdleRestitution = 0.34f;

    for (Item& item : _items) {
        if (!item.active) {
            continue;
        }

        if (item.cooldown > 0.0f && !item.triggeredJump && item.jumpOffset == 0.0f && item.jumpVelocity == 0.0f) {
            item.cooldown = std::max(0.0f, item.cooldown - dt);
        }

        if (item.jumpOffset < 0.0f || item.jumpVelocity != 0.0f) {
            item.jumpVelocity += kGiantFleaGravity * dt;
            item.jumpOffset += item.jumpVelocity * dt;
            if (item.jumpOffset > 0.0f) {
                item.jumpOffset = 0.0f;
                item.tiltVelocity +=
                    giantFleaSide(item.basePos, item.boost) * clampf(std::abs(item.jumpVelocity) * 4.2f, 8.0f, 34.0f);
                const float minBounce = item.triggeredJump ? 2.4f : 1.7f;
                const float restitution = item.triggeredJump ? kTriggeredRestitution : kIdleRestitution;
                if (std::abs(item.jumpVelocity) > minBounce) {
                    item.jumpVelocity = -item.jumpVelocity * restitution;
                } else {
                    item.jumpVelocity = 0.0f;
                    if (item.triggeredJump) {
                        item.cooldown = 0.42f + item.boost * 0.16f;
                        item.idleCooldown = 0.34f + item.boost * 0.10f;
                    } else {
                        item.idleCooldown = 0.62f + item.boost * 0.28f;
                    }
                    item.triggeredJump = false;
                }
            }
        } else if (item.cooldown <= 0.0f) {
            item.idleCooldown = std::max(0.0f, item.idleCooldown - dt);
            if (item.idleCooldown == 0.0f) {
                // Bigger idle hops than regular flea.
                const float jumpHeight = 0.30f + item.boost * 0.42f;
                item.jumpVelocity = -std::sqrt(2.0f * kGiantFleaGravity * jumpHeight);
                item.tiltVelocity += -giantFleaSide(item.basePos, item.boost) * (4.0f + item.boost * 2.6f);
            }
        }

        item.tiltVelocity += -item.tilt * 34.0f * dt;
        item.tiltVelocity *= std::pow(0.035f, dt);
        item.tilt += item.tiltVelocity * dt;
        item.tilt = clampf(item.tilt, -14.0f, 14.0f);
        item.pos = {item.basePos.x, item.basePos.y + item.jumpOffset};
        if (b2Body_IsValid(item.bodyId)) {
            b2Body_SetTransform(item.bodyId, {item.pos.x, item.pos.y}, b2MakeRot(0.0f));
        }
    }

    // --- Attached state: detect landings and trigger bounces ---
    if (!_attached || !game.carValid()) {
        return;
    }

    const bool grounded = game._vehicle.frontGrounded() || game._vehicle.rearGrounded();
    if (grounded && !_wasGrounded && _bouncesRemaining > 0) {
        // Landing detected: apply a bounce.
        applyBounce(game);
        _bouncesRemaining--;
        if (_bouncesRemaining <= 0) {
            // All bounces used --- detach with a finish burst.
            const b2Vec2 chassisPos = game._vehicle.chassisPosition();
            game._pickups.effects().spawn(
                {chassisPos.x, chassisPos.y - 0.3f}, PickupEffects::Kind::GiantFlea, game._runSeed);
            _attached = false;
            _wasGrounded = false;
            return;
        }
        _wasGrounded = true; // Prevent re-trigger on the same contact.
    } else if (!grounded) {
        _wasGrounded = false;
    }
}

float GiantFleaPickups::pickupDistance() const
{
    return kGiantFleaPickupDistance;
}

bool GiantFleaPickups::attached() const
{
    return _attached;
}

void GiantFleaPickups::draw(const RidgeDashGame& game) const
{
    // --- Draw attached giant flea on the vehicle ---
    if (_attached && game.carValid()) {
        const bool interp = game.renderInterpolation();
        const float alpha = game.renderAlpha();
        // Attach at a similar spot to the rocket: behind and above the chassis center.
        const b2Vec2 gfPos = game._vehicle.renderChassisWorldPoint({-0.08f, 0.56f}, interp, alpha);
        Vector2 p = game.worldToScreen(gfPos);
        if (interp) {
            p.y += kBodyVisualYOffset;
        } else {
            p.x = std::round(p.x);
            p.y = std::round(p.y + kBodyVisualYOffset);
        }
        const float angle = game._vehicle.renderChassisAngleDeg(interp, alpha);
        if (textureLoaded(game._sprites.giantFlea)) {
            drawSpriteCentered(game._sprites.giantFlea, p, 22.0f, 16.0f, angle);
        } else {
            // Fallback: bigger flea-like rectangle.
            DrawRectanglePro(Rectangle{p.x, p.y, 22.0f, 14.0f}, Vector2{11.0f, 7.0f}, angle, Color{94, 239, 189, 255});
            DrawRectanglePro(
                Rectangle{p.x + 8.0f, p.y - 2.0f, 8.0f, 8.0f}, Vector2{4.0f, 4.0f}, angle, Color{34, 95, 86, 255});
        }
    }

    // --- Draw terrain items ---
    for (const Item& item : _items) {
        if (!item.active) {
            continue;
        }
        const Vector2 p = game.worldToScreen({item.pos.x, item.pos.y});
        if (p.x < -22.0f || p.x > kScreenWidth + 22.0f) {
            continue;
        }

        const int ix = static_cast<int>(std::round(p.x));
        const int iy = static_cast<int>(std::round(p.y));
        if (textureLoaded(game._sprites.giantFlea)) {
            drawSpriteCentered(
                game._sprites.giantFlea, {static_cast<float>(ix), static_cast<float>(iy + 3)}, 22.0f, 16.0f, item.tilt);
            continue;
        }

        // Fallback pixel art: larger version of flea.
        const bool grounded = item.jumpOffset == 0.0f && item.jumpVelocity >= 0.0f;
        const int squash = grounded && item.cooldown > 0.0f ? 1 : 0;
        const Color body = Color{94, 239, 189, 255};
        const Color belly = Color{195, 255, 230, 255};
        const Color dark = Color{34, 95, 86, 255};
        const Color eye = Color{30, 36, 46, 255};

        DrawRectangle(ix - 7, iy - 5 + squash, 14, 9 - squash, body);
        DrawRectangle(ix - 5, iy - 8 + squash, 10, 4, belly);
        DrawRectangle(ix - 9, iy - 3, 3, 4, dark);
        DrawRectangle(ix + 6, iy - 3, 3, 4, dark);
        DrawRectangle(ix - 3, iy - 6 + squash, 2, 2, eye);
        DrawRectangle(ix + 2, iy - 6 + squash, 2, 2, eye);
        DrawRectangle(ix - 9, iy + 3, 5, 2, dark);
        DrawRectangle(ix + 4, iy + 3, 5, 2, dark);
        DrawRectangle(ix - 7, iy + 6, 4, 2, dark);
        DrawRectangle(ix + 3, iy + 6, 4, 2, dark);
        if (item.jumpOffset < -0.08f) {
            DrawRectangle(ix - 6, iy + 7, 3, 2, fadeColor(Color{210, 255, 239, 255}, 0.55f));
            DrawRectangle(ix + 3, iy + 7, 3, 2, fadeColor(Color{210, 255, 239, 255}, 0.55f));
        }
    }
}

void GiantFleaPickups::doClear()
{
    _attached = false;
    _bouncesRemaining = 0;
    _wasGrounded = false;
    _activeBoost = 1.0f;
}

} // namespace ridge_dash

/**
 * @file rocket_pickups.cpp
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

constexpr float kPi = 3.14159265358979323846f;

bool nearPoint(Vector2 a, Vector2 b, float distance)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy <= distance * distance;
}

b2Vec2 rotateVec(b2Rot rotation, b2Vec2 v)
{
    return b2RotateVector(rotation, v);
}

float hash01(uint32_t value)
{
    return static_cast<float>(value & 0xffU) / 255.0f;
}

} // namespace

using namespace game_config;

void RocketPickups::clear()
{
    _items.clear();
    _trail.clear();
    _nextX = 0.0f;
    _flightTimer = 0.0f;
    _trailRemainder = 0.0f;
    _trailSerial = 0;
    _flightActive = false;
}

void RocketPickups::reset(RidgeDashGame& game)
{
    clear();
    _items.reserve(8);
    _trail.reserve(96);

    std::uniform_real_distribution<float> gapDist(368.0f, 592.0f);
    _nextX = 54.0f + gapDist(game._rng) * 0.18f;
    stream(game, game._startX + kRocketGenerateAhead);
}

void RocketPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(368.0f, 592.0f);
    while (_nextX < targetX) {
        const TerrainSample terrain = game._terrain.sampleAt(_nextX, 12.0f, game._rng);

        if (game._pickups.fuel().activeNear(_nextX, 6.5f) || game._pickups.coin().activeNear(_nextX, 5.5f) ||
            game._pickups.flea().activeInRange(_nextX - 5.8f, _nextX + 5.8f) ||
            game._pickups.snowman().activeNear(_nextX, 6.0f)) {
            _nextX += 8.5f;
            continue;
        }

        create(game, terrain);
        _nextX += gapDist(game._rng);
    }
}

void RocketPickups::create(RidgeDashGame& game, const TerrainSample& terrain)
{
    if (!b2World_IsValid(game._worldId)) {
        return;
    }

    std::uniform_real_distribution<float> phaseDist(0.0f, kPi * 2.0f);

    _items.push_back(Item{});
    Item& rocket = _items.back();
    rocket.basePos = {terrain.x, terrain.y - 1.12f};
    rocket.pos = rocket.basePos;
    rocket.idlePhase = phaseDist(game._rng);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {rocket.pos.x, rocket.pos.y};
    rocket.bodyId = b2CreateBody(game._worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;
    b2Circle circle = {{0.0f, 0.0f}, kRocketRadius};
    rocket.shapeId = b2CreateCircleShape(rocket.bodyId, &shapeDef, &circle);
}

void RocketPickups::trim(float minX)
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

bool RocketPickups::collect(RidgeDashGame& game, Item& rocket)
{
    if (!rocket.active || _flightActive || !game.carValid()) {
        return false;
    }

    rocket.active = false;
    if (b2Body_IsValid(rocket.bodyId)) {
        b2DestroyBody(rocket.bodyId);
    }
    rocket.bodyId = b2_nullBodyId;
    rocket.shapeId = b2_nullShapeId;

    _flightActive = true;
    _flightTimer = 1.58f;
    _trailRemainder = 0.0f;
    const b2Vec2 velocity = game._vehicle.chassisVelocity();
    const b2Vec2 deltaVelocity = {3.0f + std::max(0.0f, velocity.x) * 0.12f,
                                  -(8.2f + std::max(0.0f, velocity.y) * 0.18f)};
    game._vehicle.applyChassisDeltaVelocity(deltaVelocity);
    game._vehicle.applyChassisAngularImpulse(-0.18f);
    return true;
}

void RocketPickups::spawnTrail(RidgeDashGame& game, b2Vec2 tailPos, b2Vec2 carVelocity)
{
    const uint32_t seed = game._runSeed * 197u + _trailSerial++ * 2246822519u;
    const float r0 = hash01(seed);
    const float r1 = hash01(seed >> 8U);
    const float r2 = hash01(seed >> 16U);

    TrailParticle particle{};
    particle.pos = {tailPos.x, tailPos.y};
    particle.vel = {-2.5f - r0 * 2.8f + carVelocity.x * 0.05f, 0.5f + (r1 - 0.5f) * 2.2f + carVelocity.y * 0.04f};
    particle.age = 0.0f;
    particle.life = 0.34f + r2 * 0.18f;
    particle.seed = seed;
    _trail.push_back(particle);
}

void RocketPickups::update(RidgeDashGame& game, float dt)
{
    for (Item& rocket : _items) {
        if (!rocket.active) {
            continue;
        }
        rocket.idleTime += dt;
        rocket.pos = {rocket.basePos.x, rocket.basePos.y + std::sin(rocket.idleTime * 3.4f + rocket.idlePhase) * 0.16f};
        if (b2Body_IsValid(rocket.bodyId)) {
            b2Body_SetTransform(rocket.bodyId, {rocket.pos.x, rocket.pos.y}, b2MakeRot(0.0f));
        }
    }

    auto it = _trail.begin();
    while (it != _trail.end()) {
        it->age += dt;
        it->pos.x += it->vel.x * dt;
        it->pos.y += it->vel.y * dt;
        it->vel.y += 2.8f * dt;
        it->vel.x *= std::max(0.0f, 1.0f - 1.6f * dt);
        if (it->age >= it->life) {
            it = _trail.erase(it);
        } else {
            ++it;
        }
    }

    if (!_flightActive || !game.carValid()) {
        return;
    }

    _flightTimer -= dt;
    const b2Vec2 velocity = game._vehicle.chassisVelocity();
    const float throttle = std::max(0.0f, std::min(_flightTimer / 1.58f, 1.0f));
    const b2Vec2 thrust = {11.5f + velocity.x * 0.16f, -(23.0f + throttle * 11.0f)};
    game._vehicle.applyChassisForcePerMass(thrust);
    game._vehicle.applyChassisTorque(-2.0f * throttle);

    const b2Rot rotation = game._vehicle.chassisRotation();
    const b2Vec2 center = game._vehicle.chassisWorldPoint({-0.10f, 0.58f});
    const b2Vec2 tailPos = center + rotateVec(rotation, {-1.10f, 0.02f});
    _trailRemainder += dt * 58.0f;
    while (_trailRemainder >= 1.0f) {
        spawnTrail(game, tailPos, velocity);
        _trailRemainder -= 1.0f;
    }

    if (_flightTimer <= 0.0f) {
        const b2Vec2 boom = game._vehicle.chassisWorldPoint({-0.08f, 0.58f});
        game._pickups.effects().spawn({boom.x, boom.y}, PickupEffects::Kind::Rocket, game._runSeed);
        _flightActive = false;
        _flightTimer = 0.0f;
        _trailRemainder = 0.0f;
    }
}

bool RocketPickups::collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape)
{
    if (!game._vehicle.shapeBelongsToVehicle(otherShape)) {
        return false;
    }
    for (Item& rocket : _items) {
        if (rocket.active && b2Shape_IsValid(rocket.shapeId) && B2_ID_EQUALS(rocket.shapeId, pickupShape)) {
            return collect(game, rocket);
        }
    }
    return false;
}

bool RocketPickups::collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus)
{
    bool collected = false;
    for (Item& rocket : _items) {
        if (!rocket.active) {
            continue;
        }
        for (int i = 0; i < count; ++i) {
            if (nearPoint(points[i], rocket.pos, kRocketPickupDistance + speedBonus)) {
                collected = collect(game, rocket) || collected;
                break;
            }
        }
    }
    return collected;
}

bool RocketPickups::activeNear(float x, float distance) const
{
    for (const Item& rocket : _items) {
        if (rocket.active && std::abs(rocket.basePos.x - x) <= distance) {
            return true;
        }
    }
    return false;
}

void RocketPickups::draw(const RidgeDashGame& game) const
{
    const Color trailColors[] = {
        Color{255, 233, 104, 255},
        Color{255, 120, 89, 255},
        Color{255, 248, 207, 255},
    };
    for (const TrailParticle& particle : _trail) {
        const float t = particle.age / particle.life;
        const float alpha = std::max(0.0f, 1.0f - t * t);
        const Vector2 p = game.worldToScreen({particle.pos.x, particle.pos.y});
        if (p.x < -18.0f || p.x > kScreenWidth + 18.0f || p.y < -18.0f || p.y > kScreenHeight + 18.0f) {
            continue;
        }
        const int size = t < 0.22f ? 5 : (t < 0.58f ? 4 : 2);
        const Color color = fadeColor(trailColors[particle.seed % 3U], alpha);
        DrawRectangle(static_cast<int>(std::round(p.x)) - size / 2,
                      static_cast<int>(std::round(p.y)) - size / 2,
                      size,
                      size,
                      color);
    }

    if (_flightActive && game.carValid()) {
        const b2Rot rotation = game._vehicle.chassisRotation();
        const b2Vec2 rocketPos = game._vehicle.chassisWorldPoint({-0.10f, 0.58f});
        Vector2 p = game.worldToScreen(rocketPos);
        p.x = std::round(p.x);
        p.y = std::round(p.y + kBodyVisualYOffset);
        const float angle = b2Rot_GetAngle(rotation) * RAD2DEG;
        if (textureLoaded(game._sprites.rocket)) {
            drawSpriteCentered(game._sprites.rocket, p, 31.0f, 17.0f, angle);
        } else {
            DrawRectanglePro(Rectangle{p.x, p.y, 30.0f, 8.0f}, Vector2{15.0f, 4.0f}, angle, Color{245, 245, 245, 255});
            DrawRectanglePro(
                Rectangle{p.x + 12.0f, p.y, 8.0f, 8.0f}, Vector2{4.0f, 4.0f}, angle, Color{255, 87, 87, 255});
        }
    }

    for (const Item& rocket : _items) {
        if (!rocket.active) {
            continue;
        }
        const Vector2 p = game.worldToScreen({rocket.pos.x, rocket.pos.y});
        if (p.x < -24.0f || p.x > kScreenWidth + 24.0f) {
            continue;
        }
        const float angle = std::sin(rocket.idleTime * 2.4f + rocket.idlePhase) * 5.0f;
        if (textureLoaded(game._sprites.rocket)) {
            drawSpriteCentered(game._sprites.rocket, {std::round(p.x), std::round(p.y)}, 31.0f, 17.0f, angle);
        } else {
            DrawRectanglePro(Rectangle{std::round(p.x), std::round(p.y), 30.0f, 8.0f},
                             Vector2{15.0f, 4.0f},
                             angle,
                             Color{245, 245, 245, 255});
            DrawRectanglePro(Rectangle{std::round(p.x) + 12.0f, std::round(p.y), 8.0f, 8.0f},
                             Vector2{4.0f, 4.0f},
                             angle,
                             Color{255, 87, 87, 255});
        }
    }
}

} // namespace ridge_dash

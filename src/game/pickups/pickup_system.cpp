/**
 * @file pickup_system.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/ridge_dash_game.hpp"

#include "game/game_config.hpp"

namespace ridge_dash {

using namespace game_config;

void PickupSystem::clear()
{
    _effects.clear();
    _fuel.clear();
    _coin.clear();
    _flea.clear();
    _rocket.clear();
    _cactus.clear();
    _snowman.clear();
    _giantFlea.clear();
    _helmet.clear();
    _squid.clear();
}

void PickupSystem::reset(RidgeDashGame& game)
{
    _effects.reset();
    _fuel.reset(game);
    _coin.reset(game);
    _flea.reset(game);
    _rocket.reset(game);
    _cactus.reset(game);
    _snowman.reset(game);
    _giantFlea.reset(game);
    _helmet.reset(game);
    _squid.reset();
}

void PickupSystem::update(RidgeDashGame& game, float dt)
{
    _fuel.update(dt);
    _flea.update(game, dt);
    _rocket.update(game, dt);
    _cactus.update(dt);
    _snowman.update(game, dt);
    _giantFlea.update(game, dt);
    _helmet.update(dt);
    _squid.update(dt);
}

void PickupSystem::applyStepForces(RidgeDashGame& game)
{
    _rocket.applyStepForces(game);
    _snowman.applyStepForces(game);
}

void PickupSystem::updateEffects(float dt)
{
    _effects.update(dt);
}

void PickupSystem::stream(RidgeDashGame& game, float carX)
{
    _fuel.stream(game, carX + kFuelGenerateAhead);
    _coin.stream(game, carX + kCoinGenerateAhead);
    _flea.stream(game, carX + kFleaGenerateAhead);
    _rocket.stream(game, carX + kRocketGenerateAhead);
    _cactus.stream(game, carX + kCactusGenerateAhead);
    _snowman.stream(game, carX + kSnowmanGenerateAhead);
    _giantFlea.stream(game, carX + kGiantFleaGenerateAhead);
    _helmet.stream(game, carX + kHelmetGenerateAhead);
}

void PickupSystem::trim(float minX)
{
    _fuel.trim(minX);
    _coin.trim(minX);
    _flea.trim(minX);
    _rocket.trim(minX);
    _cactus.trim(minX);
    _snowman.trim(minX);
    _giantFlea.trim(minX);
    _helmet.trim(minX);
}

bool PickupSystem::collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape)
{
    return _fuel.collectByShape(game, pickupShape, otherShape) || _coin.collectByShape(game, pickupShape, otherShape) ||
           _flea.collectByShape(game, pickupShape, otherShape) ||
           _rocket.collectByShape(game, pickupShape, otherShape) ||
           _cactus.collectByShape(game, pickupShape, otherShape) ||
           _snowman.collectByShape(game, pickupShape, otherShape) ||
           _giantFlea.collectByShape(game, pickupShape, otherShape) ||
           _helmet.collectByShape(game, pickupShape, otherShape);
}

bool PickupSystem::collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus)
{
    bool collected = false;
    collected = _fuel.collectOverlaps(game, points, count, speedBonus) || collected;
    collected = _coin.collectOverlaps(game, points, count, speedBonus) || collected;
    collected = _flea.collectOverlaps(game, points, count, speedBonus) || collected;
    collected = _rocket.collectOverlaps(game, points, count, speedBonus) || collected;
    collected = _cactus.collectOverlaps(game, points, count, speedBonus) || collected;
    collected = _snowman.collectOverlaps(game, points, count, speedBonus) || collected;
    collected = _giantFlea.collectOverlaps(game, points, count, speedBonus) || collected;
    collected = _helmet.collectOverlaps(game, points, count, speedBonus) || collected;
    return collected;
}

void PickupSystem::triggerSquid(uint32_t runSeed)
{
    _squid.trigger(runSeed);
}

void PickupSystem::draw(const RidgeDashGame& game) const
{
    _fuel.draw(game);
    _coin.draw(game);
    _flea.draw(game);
    _rocket.draw(game);
    _cactus.draw(game);
    _snowman.draw(game);
    _giantFlea.draw(game);
    _helmet.draw(game);
}

void PickupSystem::drawEffects(const RidgeDashGame& game) const
{
    _effects.draw(game);
    _squid.draw(game);
}

PickupEffects& PickupSystem::effects()
{
    return _effects;
}

FuelPickups& PickupSystem::fuel()
{
    return _fuel;
}

CoinPickups& PickupSystem::coin()
{
    return _coin;
}

FleaPickups& PickupSystem::flea()
{
    return _flea;
}

RocketPickups& PickupSystem::rocket()
{
    return _rocket;
}

CactusPickups& PickupSystem::cactus()
{
    return _cactus;
}

SnowmanPickups& PickupSystem::snowman()
{
    return _snowman;
}

SquidPickups& PickupSystem::squid()
{
    return _squid;
}

GiantFleaPickups& PickupSystem::giantFlea()
{
    return _giantFlea;
}

HelmetPickups& PickupSystem::helmet()
{
    return _helmet;
}

const PickupEffects& PickupSystem::effects() const
{
    return _effects;
}

const FuelPickups& PickupSystem::fuel() const
{
    return _fuel;
}

const CoinPickups& PickupSystem::coin() const
{
    return _coin;
}

const FleaPickups& PickupSystem::flea() const
{
    return _flea;
}

const RocketPickups& PickupSystem::rocket() const
{
    return _rocket;
}

const CactusPickups& PickupSystem::cactus() const
{
    return _cactus;
}

const SnowmanPickups& PickupSystem::snowman() const
{
    return _snowman;
}

const SquidPickups& PickupSystem::squid() const
{
    return _squid;
}

const GiantFleaPickups& PickupSystem::giantFlea() const
{
    return _giantFlea;
}

const HelmetPickups& PickupSystem::helmet() const
{
    return _helmet;
}

void RidgeDashGame::updatePickupOverlaps()
{
    if (!carValid() || _runController.gameOver()) {
        return;
    }

    Vector2 points[4]{};
    int count = 0;
    const b2Vec2 chassisPos = _vehicle.chassisPosition();
    const b2Vec2 driverPos = _vehicle.chassisWorldPoint(kDriverLocalPos);
    points[count++] = {chassisPos.x, chassisPos.y};
    points[count++] = {driverPos.x, driverPos.y};
    if (b2Body_IsValid(_vehicle.rearWheelId())) {
        points[count++] = {b2Body_GetPosition(_vehicle.rearWheelId()).x, b2Body_GetPosition(_vehicle.rearWheelId()).y};
    }
    if (b2Body_IsValid(_vehicle.frontWheelId())) {
        points[count++] = {b2Body_GetPosition(_vehicle.frontWheelId()).x,
                           b2Body_GetPosition(_vehicle.frontWheelId()).y};
    }

    const float speedBonus = clampf(length(_vehicle.chassisVelocity()) * 0.025f, 0.0f, 0.24f);
    _pickups.collectOverlaps(*this, points, count, speedBonus);
}

void RidgeDashGame::handleSensorEvents()
{
    if (!b2World_IsValid(_worldId)) {
        return;
    }

    b2SensorEvents events = b2World_GetSensorEvents(_worldId);
    auto isTerrainShape = [&](b2ShapeId shapeId) {
        return _terrain.isTerrainShape(shapeId);
    };
    auto collectPickupEvent = [&](b2ShapeId pickupShape, b2ShapeId otherShape) {
        return !_runController.gameOver() && _pickups.collectByShape(*this, pickupShape, otherShape);
    };

    for (int i = 0; i < events.beginCount; ++i) {
        const b2SensorBeginTouchEvent& event = events.beginEvents[i];
        if (collectPickupEvent(event.sensorShapeId, event.visitorShapeId) ||
            collectPickupEvent(event.visitorShapeId, event.sensorShapeId)) {
            continue;
        }

        const bool headHit =
            (B2_ID_EQUALS(event.sensorShapeId, _vehicle.headShape()) && isTerrainShape(event.visitorShapeId)) ||
            (B2_ID_EQUALS(event.visitorShapeId, _vehicle.headShape()) && isTerrainShape(event.sensorShapeId));
        if (headHit) {
            // Skip head hits while invincible (post-helmet-rescue grace period).
            if (_invincibleTimer > 0.0f) {
                continue;
            }
            // Helmet revival: instead of game over, give the car a strong rescue bounce.
            // Use a local flag so multiple terrain-segment touches in one frame don't
            // consume the helmet on the first event and then trigger game over on the next.
            if (_helmetActive && !_helmetRescuedThisFrame) {
                _helmetActive = false;
                _helmetRescuedThisFrame = true;
                const b2Vec2 velocity = _vehicle.chassisVelocity();
                // Kill most horizontal speed, keep a hint for inertia feel.
                const float keepX = velocity.x * 0.12f;
                // Upward bounce matching flea's effective range (~13-15 m/s).
                const float upwardDelta =
                    13.5f + std::max(0.0f, velocity.y) * 0.30f - std::max(0.0f, -velocity.y) * 0.06f;
                const b2Vec2 deltaVelocity = {keepX - velocity.x, -clampf(upwardDelta, 9.5f, 15.0f)};
                _vehicle.applyMainBodyDeltaVelocity(deltaVelocity, 1.0f, 1.0f, 1.0f);
                _vehicle.applyChassisAngularImpulse((velocity.x >= 0.0f ? -1.0f : 1.0f) * 0.26f);
                _vehicle.triggerDriverHitFlash();
                const b2Vec2 headWorld = _vehicle.chassisWorldPoint(game_config::kDriverLocalPos);
                _pickups.effects().spawn({headWorld.x, headWorld.y}, PickupEffects::Kind::Helmet, _runSeed);
                playSfx(AudioSystem::Sfx::FleaJump); // Placeholder — replace with helmet SFX later.
                // Brief invincibility so the car doesn't immediately re-trigger on the
                // same terrain after bouncing up.
                _invincibleTimer = 0.85f;
            } else if (!_helmetRescuedThisFrame && _runController.markHeadHit()) {
                _vehicle.triggerDriverHitFlash();
            }
        }
    }
}

} // namespace ridge_dash

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
#include "game/pickups/pickup_base_impl.hpp"

#include "game/game_config.hpp"

namespace ridge_dash {

using namespace game_config;

void PickupSystem::clear()
{
    _effects.clear();
    _squid.clear();
    forEachStandard([](auto& p) {
        p.clear();
    });
}

void PickupSystem::reset(RidgeDashGame& game)
{
    _effects.reset();
    _squid.reset();
    forEachStandard([&](auto& p) {
        p.reset(game);
    });
}

void PickupSystem::update(RidgeDashGame& game, float dt)
{
    // update() signatures vary (some need game, some don't) — keep manual dispatch.
    _fuel.update(dt);
    _flea.update(game, dt);
    _rocket.update(game, dt);
    _cactus.update(dt);
    _snowman.update(game, dt);
    _giantFlea.update(game, dt);
    _helmet.update(dt);
    _magnet.update(game, dt);
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
    forEachStandard([&](auto& p) {
        p.stream(game, carX + std::decay_t<decltype(p)>::kGenerateAhead);
    });
}

void PickupSystem::trim(float minX)
{
    forEachStandard([minX](auto& p) {
        p.trim(minX);
    });
}

bool PickupSystem::collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape)
{
    bool result = false;
    forEachStandard([&](auto& p) {
        result = p.collectByShape(game, pickupShape, otherShape) || result;
    });
    return result;
}

bool PickupSystem::collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus)
{
    bool collected = false;
    forEachStandard([&](auto& p) {
        collected = p.collectOverlaps(game, points, count, speedBonus) || collected;
    });
    return collected;
}

void PickupSystem::triggerSquid(uint32_t runSeed)
{
    _squid.trigger(runSeed);
}

void PickupSystem::draw(const RidgeDashGame& game) const
{
    forEachStandard([&](const auto& p) {
        p.draw(game);
    });
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

MagnetPickups& PickupSystem::magnet()
{
    return _magnet;
}

const MagnetPickups& PickupSystem::magnet() const
{
    return _magnet;
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

    // Pass 1 — collect ALL pickups before any head-hit check, so a helmet
    // picked up in the same frame (even from a later event in the array) is
    // always available for rescue protection.
    for (int i = 0; i < events.beginCount; ++i) {
        const b2SensorBeginTouchEvent& event = events.beginEvents[i];
        collectPickupEvent(event.sensorShapeId, event.visitorShapeId) ||
            collectPickupEvent(event.visitorShapeId, event.sensorShapeId);
    }

    // Pass 2 — detect head hits.  By now all pickups have been collected.
    for (int i = 0; i < events.beginCount; ++i) {
        const b2SensorBeginTouchEvent& event = events.beginEvents[i];

        const bool headHit =
            (B2_ID_EQUALS(event.sensorShapeId, _vehicle.headShape()) && isTerrainShape(event.visitorShapeId)) ||
            (B2_ID_EQUALS(event.visitorShapeId, _vehicle.headShape()) && isTerrainShape(event.sensorShapeId));
        if (!headHit) {
            continue;
        }

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
            // Floor at 0 so a fast backward roll doesn't trigger the
            // very short rolling-backward death delay (0.2 s) after rescue.
            float keepX = velocity.x * 0.12f;
            if (keepX < 0.0f) {
                keepX = 0.0f;
            }
            // Upward bounce matching flea's effective range (~13-15 m/s).
            const float upwardDelta = 13.5f + std::max(0.0f, velocity.y) * 0.30f - std::max(0.0f, -velocity.y) * 0.06f;
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
            // Reset the crash timer so any pre-rescue upside-down / rolling-backward
            // accumulation doesn't carry over and trigger game over immediately.
            _runController.resetCrashTimer();
        } else if (!_helmetRescuedThisFrame && _runController.markHeadHit()) {
            _vehicle.triggerDriverHitFlash();
        }
    }
}

void PickupSystem::forceSpawnTestPickup(RidgeDashGame& game, const std::string& type, float x)
{
    struct Entry {
        const char* name;
        void (*spawn)(PickupSystem&, RidgeDashGame&, float);
    };
    static const Entry kTable[] = {
        {"fuel",
         [](PickupSystem& s, RidgeDashGame& g, float xx) {
             s._fuel.forceSpawnAt(g, xx);
         }},
        {"coin",
         [](PickupSystem& s, RidgeDashGame& g, float xx) {
             s._coin.forceSpawnAt(g, xx);
         }},
        {"flea",
         [](PickupSystem& s, RidgeDashGame& g, float xx) {
             s._flea.forceSpawnAt(g, xx);
         }},
        {"rocket",
         [](PickupSystem& s, RidgeDashGame& g, float xx) {
             s._rocket.forceSpawnAt(g, xx);
         }},
        {"cactus",
         [](PickupSystem& s, RidgeDashGame& g, float xx) {
             s._cactus.forceSpawnAt(g, xx);
         }},
        {"snowman",
         [](PickupSystem& s, RidgeDashGame& g, float xx) {
             s._snowman.forceSpawnAt(g, xx);
         }},
        {"giantflea",
         [](PickupSystem& s, RidgeDashGame& g, float xx) {
             s._giantFlea.forceSpawnAt(g, xx);
         }},
        {"helmet",
         [](PickupSystem& s, RidgeDashGame& g, float xx) {
             s._helmet.forceSpawnAt(g, xx);
         }},
        {"magnet",
         [](PickupSystem& s, RidgeDashGame& g, float xx) {
             s._magnet.forceSpawnAt(g, xx);
         }},
    };
    for (const auto& entry : kTable) {
        if (type == entry.name) {
            entry.spawn(*this, game, x);
            return;
        }
    }
}

} // namespace ridge_dash

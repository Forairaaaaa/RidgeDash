/**
 * @file pickups.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "platform/raylib_compat.hpp"

#include <box2d/box2d.h>
#include <cstdint>
#include <string>
#include <vector>

namespace ridge_dash {

class RidgeDashGame;
struct TerrainSample;

class PickupEffects {
public:
    enum class Kind { Fuel, Coin, Flea, Rocket, Cactus, Snowman, GiantFlea, Helmet };

    void clear();
    void reset();
    void spawn(Vector2 pos, Kind kind, uint32_t runSeed);
    void update(float dt);
    void draw(const RidgeDashGame& game) const;

private:
    struct Puff {
        Vector2 pos{};
        float age = 0.0f;
        float life = 0.0f;
        uint32_t seed = 0;
        Kind kind = Kind::Coin;
    };

    std::vector<Puff> _puffs;
    uint32_t _serial = 0;
};

class FuelPickups {
public:
    void clear();
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void trim(float minX);
    void update(float dt);
    bool collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape);
    bool collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus);
    bool activeInRange(float minX, float maxX) const;
    bool activeNear(float x, float distance) const;
    void draw(const RidgeDashGame& game) const;
    void forceSpawnAt(RidgeDashGame& game, float x);

private:
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 basePos{};
        Vector2 pos{};
        float idleTime = 0.0f;
        float idlePhase = 0.0f;
        bool active = true;
    };

    void create(RidgeDashGame& game, const TerrainSample& terrain);
    bool collect(RidgeDashGame& game, Item& fuel);

    std::vector<Item> _items;
    float _nextX = 0.0f;
};

class CoinPickups {
public:
    void clear();
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void trim(float minX);
    bool collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape);
    bool collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus);
    bool activeNear(float x, float distance) const;
    void draw(const RidgeDashGame& game) const;
    void forceSpawnAt(RidgeDashGame& game, float x);

private:
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 pos{};
        bool active = true;
    };

    void create(RidgeDashGame& game, const TerrainSample& terrain, float yOffset);
    void createAt(RidgeDashGame& game, Vector2 worldPos);
    bool collect(RidgeDashGame& game, Item& coin);

    std::vector<Item> _items;
    float _nextX = 0.0f;
};

class FleaPickups {
public:
    void clear();
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void trim(float minX);
    void update(RidgeDashGame& game, float dt);
    bool collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape);
    bool collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus);
    bool activeInRange(float minX, float maxX) const;
    void draw(const RidgeDashGame& game) const;
    void forceSpawnAt(RidgeDashGame& game, float x);

private:
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 basePos{};
        Vector2 pos{};
        float boost = 1.0f;
        float jumpOffset = 0.0f;
        float jumpVelocity = 0.0f;
        float cooldown = 0.0f;
        float idleCooldown = 0.0f;
        float tilt = 0.0f;
        float tiltVelocity = 0.0f;
        bool triggeredJump = false;
        bool active = true;
    };

    void create(RidgeDashGame& game, const TerrainSample& terrain);
    bool collect(RidgeDashGame& game, Item& flea);
    void applyBoost(RidgeDashGame& game, float boost);

    std::vector<Item> _items;
    float _nextX = 0.0f;
};

class RocketPickups {
public:
    void clear();
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void trim(float minX);
    void update(RidgeDashGame& game, float dt);
    void applyStepForces(RidgeDashGame& game);
    bool collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape);
    bool collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus);
    bool activeNear(float x, float distance) const;
    void draw(const RidgeDashGame& game) const;
    void forceSpawnAt(RidgeDashGame& game, float x);

private:
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 basePos{};
        Vector2 pos{};
        float idleTime = 0.0f;
        float idlePhase = 0.0f;
        bool active = true;
    };

    struct TrailParticle {
        Vector2 pos{};
        Vector2 vel{};
        float age = 0.0f;
        float life = 0.0f;
        uint32_t seed = 0;
    };

    void create(RidgeDashGame& game, const TerrainSample& terrain);
    bool collect(RidgeDashGame& game, Item& rocket);
    void spawnTrail(RidgeDashGame& game, b2Vec2 tailPos, b2Vec2 carVelocity);

    std::vector<Item> _items;
    std::vector<TrailParticle> _trail;
    float _nextX = 0.0f;
    float _flightTimer = 0.0f;
    float _trailRemainder = 0.0f;
    uint32_t _trailSerial = 0;
    bool _flightActive = false;
};

class CactusPickups {
public:
    void clear();
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void trim(float minX);
    void update(float dt);
    bool collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape);
    bool collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus);
    bool activeNear(float x, float distance) const;
    void draw(const RidgeDashGame& game) const;
    void forceSpawnAt(RidgeDashGame& game, float x);

private:
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 pos{};
        float cooldown = 0.0f;
        float wobble = 0.0f;
        float wobbleVelocity = 0.0f;
        bool active = true;
    };

    struct Chip {
        Vector2 pos{};
        Vector2 vel{};
        float age = 0.0f;
        float life = 0.0f;
        uint32_t seed = 0;
    };

    void create(RidgeDashGame& game, const TerrainSample& terrain);
    bool hit(RidgeDashGame& game, Item& cactus);
    void spawnChips(RidgeDashGame& game, Vector2 pos, float direction);

    std::vector<Item> _items;
    std::vector<Chip> _chips;
    float _nextX = 0.0f;
    uint32_t _chipSerial = 0;
};

class SnowmanPickups {
public:
    void clear();
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void trim(float minX);
    void update(RidgeDashGame& game, float dt);
    void applyStepForces(RidgeDashGame& game);
    bool collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape);
    bool collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus);
    bool activeNear(float x, float distance) const;
    void draw(const RidgeDashGame& game) const;
    void forceSpawnAt(RidgeDashGame& game, float x);

private:
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 pos{};
        float boost = 1.0f;
        bool active = true;
    };

    void create(RidgeDashGame& game, const TerrainSample& terrain);
    bool collect(RidgeDashGame& game, Item& snowman);
    void applyBoost(RidgeDashGame& game);

    std::vector<Item> _items;
    float _nextX = 0.0f;
    float _boostTimer = 0.0f;
    float _activeBoost = 1.0f;
};

class SquidPickups {
public:
    void clear();
    void reset();
    void trigger(uint32_t runSeed);
    void update(float dt);
    void draw(const RidgeDashGame& game) const;

private:
    struct Squid {
        int sprite = 0;
        Vector2 start{};
        Vector2 end{};
        float baseY = 0.0f;
        float age = 0.0f;
        float duration = 0.0f;
        float arc = 0.0f;
        float phase = 0.0f;
        uint32_t seed = 0;
    };

    struct TrailParticle {
        Vector2 pos{};
        Vector2 vel{};
        Color color{};
        float age = 0.0f;
        float life = 0.0f;
        uint32_t seed = 0;
    };

    static Vector2 position(const Squid& squid);
    void spawnTrail(const Squid& squid);

    std::vector<Squid> _squids;
    std::vector<TrailParticle> _trail;
    float _trailRemainder = 0.0f;
    uint32_t _serial = 0;
};

class GiantFleaPickups {
public:
    void clear();
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void trim(float minX);
    void update(RidgeDashGame& game, float dt);
    bool collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape);
    bool collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus);
    bool activeNear(float x, float distance) const;
    bool attached() const;
    void draw(const RidgeDashGame& game) const;
    void forceSpawnAt(RidgeDashGame& game, float x);

private:
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 basePos{};
        Vector2 pos{};
        float boost = 1.0f;
        float jumpOffset = 0.0f;
        float jumpVelocity = 0.0f;
        float cooldown = 0.0f;
        float idleCooldown = 0.0f;
        float tilt = 0.0f;
        float tiltVelocity = 0.0f;
        bool triggeredJump = false;
        bool active = true;
    };

    void create(RidgeDashGame& game, const TerrainSample& terrain);
    bool collect(RidgeDashGame& game, Item& item);
    void applyBounce(RidgeDashGame& game);

    std::vector<Item> _items;
    float _nextX = 0.0f;

    // Attached state (like rocket flight, but triggered by landings).
    bool _attached = false;
    int _bouncesRemaining = 0;
    bool _wasGrounded = false;
    float _activeBoost = 1.0f;
};

class HelmetPickups {
public:
    void clear();
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void trim(float minX);
    void update(float dt);
    bool collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape);
    bool collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus);
    bool activeNear(float x, float distance) const;
    void draw(const RidgeDashGame& game) const;
    void forceSpawnAt(RidgeDashGame& game, float x);

private:
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 basePos{};
        Vector2 pos{};
        float idleTime = 0.0f;
        float idlePhase = 0.0f;
        bool active = true;
    };

    void create(RidgeDashGame& game, const TerrainSample& terrain);
    bool collect(RidgeDashGame& game, Item& item);

    std::vector<Item> _items;
    float _nextX = 0.0f;
};

class PickupSystem {
public:
    void clear();
    void reset(RidgeDashGame& game);
    void update(RidgeDashGame& game, float dt);
    void applyStepForces(RidgeDashGame& game);
    void updateEffects(float dt);
    void stream(RidgeDashGame& game, float carX);
    void trim(float minX);
    bool collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape);
    bool collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus);
    void triggerSquid(uint32_t runSeed);
    void forceSpawnTestPickup(RidgeDashGame& game, const std::string& type, float x);
    void draw(const RidgeDashGame& game) const;
    void drawEffects(const RidgeDashGame& game) const;

    PickupEffects& effects();
    FuelPickups& fuel();
    CoinPickups& coin();
    FleaPickups& flea();
    RocketPickups& rocket();
    CactusPickups& cactus();
    SnowmanPickups& snowman();
    GiantFleaPickups& giantFlea();
    HelmetPickups& helmet();
    SquidPickups& squid();

    const PickupEffects& effects() const;
    const FuelPickups& fuel() const;
    const CoinPickups& coin() const;
    const FleaPickups& flea() const;
    const RocketPickups& rocket() const;
    const CactusPickups& cactus() const;
    const SnowmanPickups& snowman() const;
    const GiantFleaPickups& giantFlea() const;
    const HelmetPickups& helmet() const;
    const SquidPickups& squid() const;

private:
    PickupEffects _effects;
    FuelPickups _fuel;
    CoinPickups _coin;
    FleaPickups _flea;
    RocketPickups _rocket;
    CactusPickups _cactus;
    SnowmanPickups _snowman;
    GiantFleaPickups _giantFlea;
    HelmetPickups _helmet;
    SquidPickups _squid;
};

} // namespace ridge_dash

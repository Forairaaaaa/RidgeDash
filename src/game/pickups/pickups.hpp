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

#include "game/pickups/pickup_base.hpp"
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

class FuelPickups : public PickupCollection<FuelPickups> {
public:
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void update(float dt);
    bool activeInRange(float minX, float maxX) const;
    void draw(const RidgeDashGame& game) const;

    // ── CRTP hooks / accessors ──────────────────────────────────────
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 basePos{};
        Vector2 pos{};
        float idleTime = 0.0f;
        float idlePhase = 0.0f;
        bool active = true;
    };

    std::vector<Item>& items()
    {
        return _items;
    }
    const std::vector<Item>& items() const
    {
        return _items;
    }
    float& nextX()
    {
        return _nextX;
    }
    const float& nextX() const
    {
        return _nextX;
    }

    Vector2 itemPos(const Item& it) const
    {
        return it.pos;
    }
    float itemTrimX(const Item& it) const
    {
        return it.basePos.x;
    }
    float pickupDistance() const;
    void doCreate(RidgeDashGame& game, const TerrainSample& terrain);
    bool doCollect(RidgeDashGame& game, Item& item);

private:
    std::vector<Item> _items;
    float _nextX = 0.0f;
};

class CoinPickups : public PickupCollection<CoinPickups> {
public:
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void draw(const RidgeDashGame& game) const;

    // ── CRTP hooks / accessors ──────────────────────────────────────
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 pos{};
        bool active = true;
    };

    std::vector<Item>& items()
    {
        return _items;
    }
    const std::vector<Item>& items() const
    {
        return _items;
    }
    float& nextX()
    {
        return _nextX;
    }
    const float& nextX() const
    {
        return _nextX;
    }

    Vector2 itemPos(const Item& it) const
    {
        return it.pos;
    }
    float itemTrimX(const Item& it) const
    {
        return it.pos.x;
    }
    float pickupDistance() const;
    void doCreate(RidgeDashGame& game, const TerrainSample& terrain);
    bool doCollect(RidgeDashGame& game, Item& item);

    // Coin-specific: create at an exact world position (no terrain sample needed).
    void createAt(RidgeDashGame& game, Vector2 worldPos);

    // Override base forceSpawnAt — coin uses heightAt + createAt instead of sampleAt.
    void forceSpawnAt(RidgeDashGame& game, float x);

private:
    std::vector<Item> _items;
    float _nextX = 0.0f;
};

class FleaPickups : public PickupCollection<FleaPickups> {
public:
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void update(RidgeDashGame& game, float dt);
    bool activeInRange(float minX, float maxX) const;
    void draw(const RidgeDashGame& game) const;

    // ── CRTP hooks / accessors ──────────────────────────────────────
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

    std::vector<Item>& items()
    {
        return _items;
    }
    const std::vector<Item>& items() const
    {
        return _items;
    }
    float& nextX()
    {
        return _nextX;
    }
    const float& nextX() const
    {
        return _nextX;
    }

    Vector2 itemPos(const Item& it) const
    {
        return it.pos;
    }
    float itemTrimX(const Item& it) const
    {
        return it.basePos.x;
    }
    float pickupDistance() const;
    void doCreate(RidgeDashGame& game, const TerrainSample& terrain);
    bool doCollect(RidgeDashGame& game, Item& item);
    void applyBoost(RidgeDashGame& game, float boost);

private:
    std::vector<Item> _items;
    float _nextX = 0.0f;
};

class RocketPickups : public PickupCollection<RocketPickups> {
public:
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void update(RidgeDashGame& game, float dt);
    void applyStepForces(RidgeDashGame& game);
    void draw(const RidgeDashGame& game) const;

    // ── CRTP hooks / accessors ──────────────────────────────────────
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

    std::vector<Item>& items()
    {
        return _items;
    }
    const std::vector<Item>& items() const
    {
        return _items;
    }
    float& nextX()
    {
        return _nextX;
    }
    const float& nextX() const
    {
        return _nextX;
    }

    Vector2 itemPos(const Item& it) const
    {
        return it.pos;
    }
    float itemTrimX(const Item& it) const
    {
        return it.basePos.x;
    }
    float pickupDistance() const;
    void doCreate(RidgeDashGame& game, const TerrainSample& terrain);
    bool doCollect(RidgeDashGame& game, Item& item);
    void doClear();

private:
    void spawnTrail(RidgeDashGame& game, b2Vec2 tailPos, b2Vec2 carVelocity);

    std::vector<Item> _items;
    std::vector<TrailParticle> _trail;
    float _nextX = 0.0f;
    float _flightTimer = 0.0f;
    float _trailRemainder = 0.0f;
    uint32_t _trailSerial = 0;
    bool _flightActive = false;
};

class CactusPickups : public PickupCollection<CactusPickups> {
public:
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void update(float dt);
    void draw(const RidgeDashGame& game) const;

    // ── CRTP hooks / accessors ──────────────────────────────────────
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

    std::vector<Item>& items()
    {
        return _items;
    }
    const std::vector<Item>& items() const
    {
        return _items;
    }
    float& nextX()
    {
        return _nextX;
    }
    const float& nextX() const
    {
        return _nextX;
    }

    Vector2 itemPos(const Item& it) const
    {
        return it.pos;
    }
    float itemTrimX(const Item& it) const
    {
        return it.pos.x;
    }
    float pickupDistance() const;
    void doCreate(RidgeDashGame& game, const TerrainSample& terrain);
    bool doCollect(RidgeDashGame& game, Item& item);
    void doClear();
    void doTrimExtra(float minX);

private:
    void spawnChips(RidgeDashGame& game, Vector2 pos, float direction);

    std::vector<Item> _items;
    std::vector<Chip> _chips;
    float _nextX = 0.0f;
    uint32_t _chipSerial = 0;
};

class SnowmanPickups : public PickupCollection<SnowmanPickups> {
public:
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void update(RidgeDashGame& game, float dt);
    void applyStepForces(RidgeDashGame& game);
    void draw(const RidgeDashGame& game) const;

    // ── CRTP hooks / accessors ──────────────────────────────────────
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 pos{};
        float boost = 1.0f;
        bool active = true;
    };

    std::vector<Item>& items()
    {
        return _items;
    }
    const std::vector<Item>& items() const
    {
        return _items;
    }
    float& nextX()
    {
        return _nextX;
    }
    const float& nextX() const
    {
        return _nextX;
    }

    Vector2 itemPos(const Item& it) const
    {
        return it.pos;
    }
    float itemTrimX(const Item& it) const
    {
        return it.pos.x;
    }
    float pickupDistance() const;
    void doCreate(RidgeDashGame& game, const TerrainSample& terrain);
    bool doCollect(RidgeDashGame& game, Item& item);
    void doClear();

private:
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

class GiantFleaPickups : public PickupCollection<GiantFleaPickups> {
public:
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void update(RidgeDashGame& game, float dt);
    bool attached() const;
    void draw(const RidgeDashGame& game) const;

    // ── CRTP hooks / accessors ──────────────────────────────────────
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

    std::vector<Item>& items()
    {
        return _items;
    }
    const std::vector<Item>& items() const
    {
        return _items;
    }
    float& nextX()
    {
        return _nextX;
    }
    const float& nextX() const
    {
        return _nextX;
    }

    Vector2 itemPos(const Item& it) const
    {
        return it.pos;
    }
    float itemTrimX(const Item& it) const
    {
        return it.basePos.x;
    }
    float pickupDistance() const;
    void doCreate(RidgeDashGame& game, const TerrainSample& terrain);
    bool doCollect(RidgeDashGame& game, Item& item);
    void doClear();

private:
    void applyBounce(RidgeDashGame& game);

    std::vector<Item> _items;
    float _nextX = 0.0f;

    // Attached state (like rocket flight, but triggered by landings).
    bool _attached = false;
    int _bouncesRemaining = 0;
    bool _wasGrounded = false;
    float _activeBoost = 1.0f;
};

class HelmetPickups : public PickupCollection<HelmetPickups> {
public:
    void reset(RidgeDashGame& game);
    void stream(RidgeDashGame& game, float targetX);
    void update(float dt);
    void draw(const RidgeDashGame& game) const;

    // ── CRTP hooks / accessors (public — base calls them) ────────────
    struct Item {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        Vector2 basePos{};
        Vector2 pos{};
        float idleTime = 0.0f;
        float idlePhase = 0.0f;
        bool active = true;
    };

    std::vector<Item>& items()
    {
        return _items;
    }
    const std::vector<Item>& items() const
    {
        return _items;
    }
    float& nextX()
    {
        return _nextX;
    }
    const float& nextX() const
    {
        return _nextX;
    }

    Vector2 itemPos(const Item& it) const
    {
        return it.pos;
    }
    float itemTrimX(const Item& it) const
    {
        return it.basePos.x;
    }
    float pickupDistance() const;

    void doCreate(RidgeDashGame& game, const TerrainSample& terrain);
    bool doCollect(RidgeDashGame& game, Item& item);

private:
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

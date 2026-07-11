/**
 * @file vehicle.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "game/world/terrain_profiles.hpp"
#include "platform/raylib_compat.hpp"

#include <box2d/box2d.h>
#include <cstdint>
#include <vector>

namespace ridge_dash {

class Vehicle {
public:
    enum class DriverFace { Normal, Blink, Cried, Scared, Shocked };

    struct DriveInput {
        bool throttle = false;
        bool brake = false;
        bool canDrive = false;
    };

    struct DriveResult {
        float fuelBurnScale = 0.0f;
        bool acted = false;
    };

    struct DriverExpression {
        DriverFace face = DriverFace::Normal;
        bool hitFlash = false;
    };

    struct DrawTextures {
        Texture2D carBodyDay{};
        Texture2D carBodyNight{};
        Texture2D driver{};
        Texture2D driverBlink{};
        Texture2D driverCried{};
        Texture2D driverScared{};
        Texture2D driverShocked{};
        Texture2D wheelDay{};
        Texture2D wheelNight{};
    };

    struct DrawContext {
        DrawTextures textures{};
        Vector2 camera{};
        float dayAmount = 0.0f;
        uint32_t runSeed = 0;
        bool gameOver = false;
        bool headHit = false;
        bool interpolate = false;
        float alpha = 1.0f;
    };

    void reset();
    void create(b2WorldId worldId, float startX, float startY);
    void recordPhysicsSnapshot();
    void updateGroundState();
    DriveResult applyDriveInput(const DriveInput& input);
    void updateDriverExpression(float dt, float groundY, bool gameOver);
    void triggerDriverHitFlash();
    DriverExpression driverExpression(uint32_t seed, bool gameOver, bool headHit) const;
    void updateDust(float dt, bool running, uint32_t runSeed, TerrainBiome rearBiome, TerrainBiome frontBiome);
    void drawDust(Vector2 camera) const;
    void draw(const DrawContext& context) const;

    // Applies continuous per-physics-step forces (drive air-control torque). Called
    // once per fixed step from stepPhysics so total impulse is framerate-independent.
    void applyStepForces();

    // Interpolation-aware chassis accessors for render code attached to the vehicle
    // (e.g. the in-flight rocket sprite) so it tracks the smoothed body, not the
    // 60Hz-snapping live transform. interp==false returns the live value.
    b2Vec2 renderChassisWorldPoint(b2Vec2 localPoint, bool interp, float alpha) const;
    float renderChassisAngleDeg(bool interp, float alpha) const;
    float renderDistanceFrom(float startX, bool interp, float alpha) const;

    bool valid() const;
    bool shapeBelongsToVehicle(b2ShapeId shapeId) const;
    float distanceFrom(float startX) const;
    b2Vec2 chassisPosition() const;
    b2Vec2 chassisVelocity() const;
    float chassisAngularVelocity() const;
    b2Rot chassisRotation() const;
    b2Vec2 chassisWorldPoint(b2Vec2 localPoint) const;
    b2Vec2 frontWheelPosition() const;
    b2Vec2 rearWheelPosition() const;
    void applyChassisDeltaVelocity(b2Vec2 deltaVelocity);
    void applyMainBodyDeltaVelocity(b2Vec2 deltaVelocity,
                                    float chassisScale,
                                    float frontWheelScale,
                                    float rearWheelScale);
    void applyChassisForcePerMass(b2Vec2 forcePerMass);
    void applyMainBodyForcePerMass(b2Vec2 forcePerMass,
                                   float chassisScale,
                                   float frontWheelScale,
                                   float rearWheelScale);
    void applyChassisAngularImpulse(float impulse);
    void applyChassisTorque(float torque);

    b2BodyId chassisId() const;
    b2BodyId driverHeadId() const;
    b2BodyId frontWheelId() const;
    b2BodyId rearWheelId() const;
    b2ShapeId chassisShape() const;
    b2ShapeId headShape() const;
    b2ShapeId frontShape() const;
    b2ShapeId rearShape() const;
    bool frontGrounded() const;
    bool rearGrounded() const;

private:
    struct DustParticle {
        Vector2 pos{};
        Vector2 vel{};
        float age = 0.0f;
        float life = 0.0f;
        uint32_t seed = 0;
        TerrainBiome biome = TerrainBiome::Mountain;
    };

    void setWheelMotor(float speed, float torque);
    bool wheelGrounded(b2ShapeId shapeId) const;
    void spawnWheelDust(b2BodyId wheelId, TerrainBiome biome, uint32_t runSeed);
    void spawnLandingDust(b2BodyId wheelId, TerrainBiome biome, uint32_t runSeed, float intensity);

    b2BodyId _chassisId = b2_nullBodyId;
    b2BodyId _driverHeadId = b2_nullBodyId;
    b2BodyId _frontWheelId = b2_nullBodyId;
    b2BodyId _rearWheelId = b2_nullBodyId;
    b2JointId _driverHeadJointId = b2_nullJointId;
    b2JointId _frontJointId = b2_nullJointId;
    b2JointId _rearJointId = b2_nullJointId;
    b2ShapeId _chassisShape = b2_nullShapeId;
    b2ShapeId _headShape = b2_nullShapeId;
    b2ShapeId _frontShape = b2_nullShapeId;
    b2ShapeId _rearShape = b2_nullShapeId;
    float _driverTime = 0.0f;
    float _driverAirHeightMax = 0.0f;
    float _driverHeightAboveGround = 0.0f;
    float _driverScaredTimer = 0.0f;
    float _driverHitFlashTimer = 0.0f;
    float _pendingDriveTorque = 0.0f;
    std::vector<DustParticle> _dustParticles;
    float _dustEmitRemainder = 0.0f;
    uint32_t _dustSerial = 0;
    bool _wasAirborne = false;
    float _airFallSpeed = 0.0f;
    bool _frontGrounded = false;
    bool _rearGrounded = false;

    // Render interpolation snapshots. `_snapshot[*].prev`/`.cur` hold the two most
    // recent 60Hz physics states so draw() can interpolate between them when the
    // render frame rate exceeds the physics step. Positions are interpolated for all
    // four bodies; only chassis/head rotations are stored (wheels spin too fast to
    // interpolate — draw reads their live rotation instead).
    struct BodySnapshot {
        b2Vec2 prevPos{};
        b2Vec2 curPos{};
        b2Rot prevRot = b2MakeRot(0.0f);
        b2Rot curRot = b2MakeRot(0.0f);
    };
    BodySnapshot _chassisSnapshot;
    BodySnapshot _headSnapshot;
    BodySnapshot _frontWheelSnapshot;
    BodySnapshot _rearWheelSnapshot;
    bool _hasSnapshot = false;
};

} // namespace ridge_dash

/**
 * @file vehicle.cpp
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
#include "game/render_helpers.hpp"
#include "game/vehicle/vehicle.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ridge_dash {
namespace {

Color dustColorForBiome(TerrainBiome biome, uint32_t index)
{
    const Color mountain[] = {
        Color{214, 173, 112, 255},
        Color{176, 132, 86, 255},
        Color{124, 96, 68, 255},
    };
    const Color stone[] = {
        Color{174, 181, 188, 255},
        Color{129, 136, 145, 255},
        Color{83, 90, 101, 255},
    };
    const Color desert[] = {
        Color{255, 231, 139, 255},
        Color{246, 196, 92, 255},
        Color{221, 151, 70, 255},
    };
    const Color snow[] = {
        Color{255, 255, 255, 255},
        Color{226, 247, 255, 255},
        Color{180, 222, 239, 255},
    };

    switch (biome) {
        case TerrainBiome::Stone:
            return stone[index % 3U];
        case TerrainBiome::Desert:
            return desert[index % 3U];
        case TerrainBiome::Snow:
            return snow[index % 3U];
        case TerrainBiome::Mountain:
        default:
            return mountain[index % 3U];
    }
}

void applyDeltaVelocityToBody(b2BodyId bodyId, b2Vec2 deltaVelocity, float scale)
{
    if (!b2Body_IsValid(bodyId)) {
        return;
    }
    const float mass = b2Body_GetMass(bodyId);
    b2Body_ApplyLinearImpulseToCenter(bodyId, {deltaVelocity.x * mass * scale, deltaVelocity.y * mass * scale}, true);
}

void applyForcePerMassToBody(b2BodyId bodyId, b2Vec2 forcePerMass, float scale)
{
    if (!b2Body_IsValid(bodyId)) {
        return;
    }
    const float mass = b2Body_GetMass(bodyId);
    b2Body_ApplyForceToCenter(bodyId, {forcePerMass.x * mass * scale, forcePerMass.y * mass * scale}, true);
}

Vector2 vehicleWorldToScreen(b2Vec2 p, Vector2 camera)
{
    return {p.x * game_config::kPixelsPerMeter - camera.x, p.y * game_config::kPixelsPerMeter - camera.y};
}

} // namespace

using namespace game_config;

bool driverBlinking(uint32_t seed, float time)
{
    constexpr float kBlinkPeriod = 4.2f;
    constexpr float kBlinkWindow = 0.052f;
    const float shiftedTime = std::max(0.0f, time + static_cast<float>(seed % 997U) / 997.0f * kBlinkPeriod);
    const float local = std::fmod(shiftedTime, kBlinkPeriod) / kBlinkPeriod;
    return local <= kBlinkWindow;
}

bool driverHitFlashOn(float timer)
{
    constexpr float kFlashDuration = 0.56f;
    constexpr float kFlashSegment = kFlashDuration / 4.0f;
    if (timer <= 0.0f || timer > kFlashDuration) {
        return false;
    }

    const float elapsed = kFlashDuration - timer;
    return (static_cast<int>(elapsed / kFlashSegment) % 2) == 0;
}

void Vehicle::reset()
{
    _chassisId = b2_nullBodyId;
    _driverHeadId = b2_nullBodyId;
    _frontWheelId = b2_nullBodyId;
    _rearWheelId = b2_nullBodyId;
    _driverHeadJointId = b2_nullJointId;
    _frontJointId = b2_nullJointId;
    _rearJointId = b2_nullJointId;
    _chassisShape = b2_nullShapeId;
    _headShape = b2_nullShapeId;
    _frontShape = b2_nullShapeId;
    _rearShape = b2_nullShapeId;
    _driverTime = 0.0f;
    _driverAirHeightMax = 0.0f;
    _driverHeightAboveGround = 0.0f;
    _driverScaredTimer = 0.0f;
    _driverHitFlashTimer = 0.0f;
    _pendingDriveTorque = 0.0f;
    _dustParticles.clear();
    _dustEmitRemainder = 0.0f;
    _dustSerial = 0;
    _wasAirborne = false;
    _airFallSpeed = 0.0f;
    _landingEvent = false;
    _landingFallSpeed = 0.0f;
    _frontGrounded = false;
    _rearGrounded = false;
    _hasSnapshot = false;
}

void Vehicle::recordPhysicsSnapshot()
{
    if (!valid()) {
        _hasSnapshot = false;
        return;
    }

    const b2Vec2 chassisPos = b2Body_GetPosition(_chassisId);
    const b2Rot chassisRot = b2Body_GetRotation(_chassisId);
    const bool headValid = b2Body_IsValid(_driverHeadId);
    const b2Vec2 headPos =
        headValid ? b2Body_GetPosition(_driverHeadId) : b2Body_GetWorldPoint(_chassisId, kDriverLocalPos);
    const b2Rot headRot = headValid ? b2Body_GetRotation(_driverHeadId) : chassisRot;
    const bool frontValid = b2Body_IsValid(_frontWheelId);
    const bool rearValid = b2Body_IsValid(_rearWheelId);
    const b2Vec2 frontPos = frontValid ? b2Body_GetPosition(_frontWheelId) : chassisPos;
    const b2Vec2 rearPos = rearValid ? b2Body_GetPosition(_rearWheelId) : chassisPos;

    if (!_hasSnapshot) {
        // First sample after (re)creation: seed prev == cur so the initial frame
        // renders at the true position instead of lerping from a stale origin.
        _chassisSnapshot = {chassisPos, chassisPos, chassisRot, chassisRot};
        _headSnapshot = {headPos, headPos, headRot, headRot};
        _frontWheelSnapshot = {frontPos, frontPos, chassisRot, chassisRot};
        _rearWheelSnapshot = {rearPos, rearPos, chassisRot, chassisRot};
        _hasSnapshot = true;
        return;
    }

    _chassisSnapshot.prevPos = _chassisSnapshot.curPos;
    _chassisSnapshot.prevRot = _chassisSnapshot.curRot;
    _chassisSnapshot.curPos = chassisPos;
    _chassisSnapshot.curRot = chassisRot;

    _headSnapshot.prevPos = _headSnapshot.curPos;
    _headSnapshot.prevRot = _headSnapshot.curRot;
    _headSnapshot.curPos = headPos;
    _headSnapshot.curRot = headRot;

    _frontWheelSnapshot.prevPos = _frontWheelSnapshot.curPos;
    _frontWheelSnapshot.curPos = frontPos;

    _rearWheelSnapshot.prevPos = _rearWheelSnapshot.curPos;
    _rearWheelSnapshot.curPos = rearPos;
}

void Vehicle::create(b2WorldId worldId, float startX, float startY)
{
    reset();
    b2BodyDef chassisDef = b2DefaultBodyDef();
    chassisDef.type = b2_dynamicBody;
    chassisDef.position = {startX, startY};
    chassisDef.rotation = b2MakeRot(-0.05f);
    chassisDef.linearDamping = 0.009f;
    chassisDef.angularDamping = 0.012f;
    // Never sleep: box2d ignores wheel-joint motors on a sleeping body, which would
    // freeze the car after a few idle seconds even though input is being read.
    chassisDef.enableSleep = false;
    _chassisId = b2CreateBody(worldId, &chassisDef);

    b2ShapeDef chassisShape = b2DefaultShapeDef();
    chassisShape.density = 0.62f;
    chassisShape.material.friction = 0.48f;
    chassisShape.material.restitution = 0.16f;
    chassisShape.enableSensorEvents = true;
    b2Polygon chassisBox = b2MakeBox(1.08f, 0.28f);
    _chassisShape = b2CreatePolygonShape(_chassisId, &chassisShape, &chassisBox);

    b2ShapeDef headShape = b2DefaultShapeDef();
    headShape.isSensor = true;
    headShape.enableSensorEvents = true;
    b2Circle headCircle = {kDriverLocalPos, kHeadRadius};
    _headShape = b2CreateCircleShape(_chassisId, &headShape, &headCircle);

    const b2Vec2 driverHeadStart = b2Body_GetWorldPoint(_chassisId, kDriverLocalPos);
    b2BodyDef driverHeadDef = b2DefaultBodyDef();
    driverHeadDef.type = b2_dynamicBody;
    driverHeadDef.position = driverHeadStart;
    driverHeadDef.rotation = b2Body_GetRotation(_chassisId);
    driverHeadDef.linearDamping = 0.12f;
    driverHeadDef.angularDamping = 0.18f;
    driverHeadDef.enableSleep = false;
    _driverHeadId = b2CreateBody(worldId, &driverHeadDef);

    b2ShapeDef driverHeadShape = b2DefaultShapeDef();
    driverHeadShape.density = 0.12f;
    driverHeadShape.material.friction = 0.0f;
    driverHeadShape.material.restitution = 0.0f;
    driverHeadShape.filter.maskBits = 0;
    b2Circle driverHeadCircle = {{0.0f, 0.0f}, kHeadRadius};
    b2CreateCircleShape(_driverHeadId, &driverHeadShape, &driverHeadCircle);

    b2WeldJointDef driverHeadJoint = b2DefaultWeldJointDef();
    driverHeadJoint.bodyIdA = _chassisId;
    driverHeadJoint.bodyIdB = _driverHeadId;
    driverHeadJoint.localAnchorA = kDriverLocalPos;
    driverHeadJoint.localAnchorB = {0.0f, 0.0f};
    driverHeadJoint.referenceAngle = 0.0f;
    driverHeadJoint.linearHertz = 3.4f;
    driverHeadJoint.angularHertz = 1.8f;
    driverHeadJoint.linearDampingRatio = 0.22f;
    driverHeadJoint.angularDampingRatio = 0.28f;
    driverHeadJoint.collideConnected = false;
    _driverHeadJointId = b2CreateWeldJoint(worldId, &driverHeadJoint);

    auto createWheel = [&](float xOffset, b2BodyId& wheelId, b2ShapeId& shapeId) {
        b2BodyDef wheelDef = b2DefaultBodyDef();
        wheelDef.type = b2_dynamicBody;
        wheelDef.position = {startX + xOffset, startY + 0.58f};
        wheelDef.linearDamping = 0.009f;
        wheelDef.angularDamping = 0.009f;
        wheelDef.enableSleep = false;
        wheelId = b2CreateBody(worldId, &wheelDef);

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 0.58f;
        shapeDef.material.friction = 1.32f;
        shapeDef.material.restitution = 0.12f;
        shapeDef.enableSensorEvents = true;
        b2Circle circle = {{0.0f, 0.0f}, kWheelRadius};
        shapeId = b2CreateCircleShape(wheelId, &shapeDef, &circle);
    };

    createWheel(-0.78f, _rearWheelId, _rearShape);
    createWheel(0.82f, _frontWheelId, _frontShape);

    auto createSuspension = [&](float xOffset, b2BodyId wheelId) {
        b2WheelJointDef jointDef = b2DefaultWheelJointDef();
        jointDef.bodyIdA = _chassisId;
        jointDef.bodyIdB = wheelId;
        jointDef.localAnchorA = {xOffset, 0.46f};
        jointDef.localAnchorB = {0.0f, 0.0f};
        jointDef.localAxisA = {0.0f, 1.0f};
        jointDef.enableSpring = true;
        jointDef.hertz = 2.55f;
        jointDef.dampingRatio = 0.30f;
        jointDef.enableLimit = true;
        jointDef.lowerTranslation = -0.42f;
        jointDef.upperTranslation = 0.31f;
        jointDef.enableMotor = true;
        jointDef.maxMotorTorque = kMotorTorque;
        return b2CreateWheelJoint(worldId, &jointDef);
    };

    _rearJointId = createSuspension(-0.78f, _rearWheelId);
    _frontJointId = createSuspension(0.82f, _frontWheelId);
}

void Vehicle::setWheelMotor(float speed, float torque)
{
    if (b2Joint_IsValid(_rearJointId)) {
        b2WheelJoint_EnableMotor(_rearJointId, torque > 0.0f);
        b2WheelJoint_SetMaxMotorTorque(_rearJointId, torque);
        b2WheelJoint_SetMotorSpeed(_rearJointId, speed);
    }
    if (b2Joint_IsValid(_frontJointId)) {
        b2WheelJoint_EnableMotor(_frontJointId, torque > 0.0f);
        b2WheelJoint_SetMaxMotorTorque(_frontJointId, torque * 0.28f);
        b2WheelJoint_SetMotorSpeed(_frontJointId, speed);
    }
}

Vehicle::DriveResult Vehicle::applyDriveInput(const DriveInput& input)
{
    _pendingDriveTorque = 0.0f;
    if (!input.canDrive) {
        setWheelMotor(0.0f, 0.0f);
        return {};
    }

    const bool partlyAirborne = !_frontGrounded || !_rearGrounded;
    const float chassisAngle = b2Rot_GetAngle(chassisRotation());
    const bool postureRisk = std::abs(chassisAngle) > 0.28f;
    const bool canTilt = partlyAirborne || postureRisk;
    const float tiltTorque = partlyAirborne ? kAirControlTorque : kAirControlTorque * 0.34f;
    if (input.throttle) {
        setWheelMotor(kThrottleSpeed, kMotorTorque);
        // Store the tilt torque; it is applied once per physics step in
        // applyStepForces so its net effect is framerate-independent.
        if (canTilt) {
            _pendingDriveTorque = -tiltTorque;
        }
        return {1.0f, true};
    }
    if (input.brake) {
        setWheelMotor(kBrakeSpeed, kMotorTorque * 0.82f);
        if (canTilt) {
            _pendingDriveTorque = tiltTorque;
        }
        return {0.75f, true};
    }

    setWheelMotor(0.0f, 0.0f);
    return {};
}

void Vehicle::applyStepForces()
{
    if (_pendingDriveTorque != 0.0f) {
        applyChassisTorque(_pendingDriveTorque);
    }
}

void Vehicle::updateDriverExpression(float dt, float groundY, bool gameOver)
{
    _driverTime += dt;
    _driverScaredTimer = std::max(0.0f, _driverScaredTimer - dt);
    _driverHitFlashTimer = std::max(0.0f, _driverHitFlashTimer - dt);
    if (!valid() || gameOver) {
        _driverAirHeightMax = 0.0f;
        _driverHeightAboveGround = 0.0f;
        return;
    }

    const b2Vec2 pos = chassisPosition();
    _driverHeightAboveGround = std::max(0.0f, groundY - pos.y);
    const bool grounded = _frontGrounded || _rearGrounded;

    if (!grounded) {
        _driverAirHeightMax = std::max(_driverAirHeightMax, _driverHeightAboveGround);
        return;
    }

    if (_driverAirHeightMax > 4.4f) {
        _driverScaredTimer = 0.58f;
    }
    _driverAirHeightMax = 0.0f;
}

void Vehicle::triggerDriverHitFlash()
{
    _driverHitFlashTimer = 0.56f;
}

Vehicle::DriverExpression Vehicle::driverExpression(uint32_t seed, bool gameOver, bool headHit) const
{
    DriverExpression expression{};
    if (gameOver || headHit) {
        expression.face = DriverFace::Cried;
    } else if (_driverHeightAboveGround > 5.1f) {
        expression.face = DriverFace::Shocked;
    } else if (_driverScaredTimer > 0.0f) {
        expression.face = DriverFace::Scared;
    } else if (driverBlinking(seed, _driverTime)) {
        expression.face = DriverFace::Blink;
    }
    expression.hitFlash = driverHitFlashOn(_driverHitFlashTimer);
    return expression;
}

void Vehicle::spawnWheelDust(b2BodyId wheelId, TerrainBiome biome, uint32_t runSeed)
{
    if (!b2Body_IsValid(wheelId) || !valid()) {
        return;
    }

    const b2Vec2 wheelPos = b2Body_GetPosition(wheelId);
    const b2Vec2 carVel = chassisVelocity();
    const uint32_t seed = runSeed * 131u + _dustSerial++ * 2654435761u;
    const float r0 = static_cast<float>(seed & 0xffU) / 255.0f;
    const float r1 = static_cast<float>((seed >> 8U) & 0xffU) / 255.0f;
    const float r2 = static_cast<float>((seed >> 16U) & 0xffU) / 255.0f;
    const float trailDir = carVel.x >= 0.0f ? -1.0f : 1.0f;

    DustParticle dust{};
    dust.pos = {wheelPos.x + trailDir * (0.34f + r0 * 0.32f), wheelPos.y + 0.22f + r1 * 0.12f};
    dust.vel = {trailDir * (1.15f + r1 * 1.45f), -(0.72f + r2 * 0.95f)};
    dust.age = 0.0f;
    dust.life = 0.58f + r0 * 0.38f;
    dust.seed = seed;
    dust.biome = biome;
    _dustParticles.push_back(dust);
}

void Vehicle::spawnLandingDust(b2BodyId wheelId, TerrainBiome biome, uint32_t runSeed, float intensity)
{
    if (!b2Body_IsValid(wheelId) || !valid()) {
        return;
    }

    const b2Vec2 wheelPos = b2Body_GetPosition(wheelId);
    // Symmetric splash: more, faster, wider-spread particles than the trailing
    // plume, scaled by how hard the wheel came down (intensity in ~0..1).
    const int count = 5 + static_cast<int>(std::round(intensity * 7.0f));
    for (int i = 0; i < count; ++i) {
        const uint32_t seed = runSeed * 131u + _dustSerial++ * 2654435761u;
        const float r0 = static_cast<float>(seed & 0xffU) / 255.0f;
        const float r1 = static_cast<float>((seed >> 8U) & 0xffU) / 255.0f;
        const float r2 = static_cast<float>((seed >> 16U) & 0xffU) / 255.0f;
        // Fan out mostly sideways with a slight upward lift, both directions.
        const float side = (r0 < 0.5f ? -1.0f : 1.0f);
        const float spread = 1.6f + intensity * 2.4f;

        DustParticle dust{};
        dust.pos = {wheelPos.x + side * (0.12f + r1 * 0.30f), wheelPos.y + 0.24f + r2 * 0.10f};
        dust.vel = {side * (0.9f + r1 * spread), -(0.35f + r2 * (0.75f + intensity * 0.9f))};
        dust.age = 0.0f;
        dust.life = 0.42f + r0 * 0.40f;
        dust.seed = seed;
        dust.biome = biome;
        _dustParticles.push_back(dust);
    }
}

void Vehicle::updateDust(float dt, bool running, uint32_t runSeed, TerrainBiome rearBiome, TerrainBiome frontBiome)
{
    auto it = _dustParticles.begin();
    while (it != _dustParticles.end()) {
        it->age += dt;
        it->pos.x += it->vel.x * dt;
        it->pos.y += it->vel.y * dt;
        it->vel.y += 1.45f * dt;
        it->vel.x *= std::max(0.0f, 1.0f - 1.05f * dt);
        if (it->age >= it->life) {
            it = _dustParticles.erase(it);
        } else {
            ++it;
        }
    }

    if (!running || !valid()) {
        _dustEmitRemainder = 0.0f;
        _wasAirborne = false;
        _airFallSpeed = 0.0f;
        return;
    }

    // Landing splash: while both wheels are off the ground, remember the peak
    // downward speed; the frame the car touches down again, burst dust from the
    // wheel that just made contact, scaled by how hard it hit.
    const bool grounded = _rearGrounded || _frontGrounded;
    const float fallSpeed = std::max(0.0f, chassisVelocity().y);
    if (!grounded) {
        _wasAirborne = true;
        _airFallSpeed = std::max(_airFallSpeed, fallSpeed);
    } else {
        if (_wasAirborne) {
            // Record the touchdown for the audio layer (sound thresholds live
            // there); dust keeps its own heavier threshold below.
            _landingEvent = true;
            _landingFallSpeed = _airFallSpeed;
            if (_airFallSpeed > 3.0f) {
                const float intensity = clampf((_airFallSpeed - 3.0f) / 12.0f, 0.0f, 1.0f);
                if (_rearGrounded) {
                    spawnLandingDust(_rearWheelId, rearBiome, runSeed, intensity);
                }
                if (_frontGrounded) {
                    spawnLandingDust(_frontWheelId, frontBiome, runSeed, intensity);
                }
            }
        }
        _wasAirborne = false;
        _airFallSpeed = 0.0f;
    }

    const b2Vec2 carVel = chassisVelocity();
    const float speed = length(carVel);
    if (speed < 0.55f || (!_rearGrounded && !_frontGrounded)) {
        _dustEmitRemainder = 0.0f;
        return;
    }

    const float groundedScale = (_rearGrounded && _frontGrounded) ? 1.0f : 0.66f;
    _dustEmitRemainder += dt * clampf(10.0f + speed * 2.85f, 14.0f, 42.0f) * groundedScale;
    while (_dustEmitRemainder >= 1.0f) {
        if (_rearGrounded && (!_frontGrounded || (_dustSerial & 1U) == 0U)) {
            spawnWheelDust(_rearWheelId, rearBiome, runSeed);
        } else if (_frontGrounded) {
            spawnWheelDust(_frontWheelId, frontBiome, runSeed);
        }
        if (speed > 3.0f && _rearGrounded && (_dustSerial & 3U) == 0U) {
            spawnWheelDust(_rearWheelId, rearBiome, runSeed);
        }
        _dustEmitRemainder -= 1.0f;
    }
}

void Vehicle::drawDust(Vector2 camera) const
{
    for (const DustParticle& dust : _dustParticles) {
        const float t = dust.age / dust.life;
        const float alpha = std::max(0.0f, (1.0f - t * t) * 0.80f);
        const Vector2 p = vehicleWorldToScreen({dust.pos.x, dust.pos.y}, camera);
        if (p.x < -12.0f || p.x > kScreenWidth + 12.0f || p.y < -12.0f || p.y > kScreenHeight + 12.0f) {
            continue;
        }
        const int size = t < 0.18f ? 4 : (t < 0.58f ? 3 : (t < 0.84f ? 2 : 1));
        const Color color = fadeColor(dustColorForBiome(dust.biome, dust.seed), alpha);
        DrawRectangle(static_cast<int>(std::round(p.x)) - size / 2,
                      static_cast<int>(std::round(p.y)) - size / 2,
                      size,
                      size,
                      color);
    }
}

void Vehicle::draw(const DrawContext& context) const
{
    if (!valid()) {
        return;
    }

    const bool driverHeadValid = b2Body_IsValid(_driverHeadId);
    const bool interp = context.interpolate && _hasSnapshot;
    const float alpha = context.alpha;

    // When interpolating, blend the two most recent physics snapshots and keep
    // sub-pixel coordinates (supersampling keeps it crisp). Otherwise read live
    // box2d state and round to whole pixels exactly as before.
    const b2Vec2 chassisPos =
        interp ? b2Lerp(_chassisSnapshot.prevPos, _chassisSnapshot.curPos, alpha) : chassisPosition();
    const b2Rot chassisRot =
        interp ? b2NLerp(_chassisSnapshot.prevRot, _chassisSnapshot.curRot, alpha) : chassisRotation();
    const float chassisAngle = b2Rot_GetAngle(chassisRot) * RAD2DEG;
    Vector2 chassis = vehicleWorldToScreen(chassisPos, context.camera);
    if (interp) {
        chassis.y += kBodyVisualYOffset;
    } else {
        chassis.x = std::round(chassis.x);
        chassis.y = std::round(chassis.y + kBodyVisualYOffset);
    }

    const b2Vec2 headWorld = interp ? b2Lerp(_headSnapshot.prevPos, _headSnapshot.curPos, alpha)
                                    : (driverHeadValid ? b2Body_GetPosition(_driverHeadId)
                                                       : b2Body_GetWorldPoint(_chassisId, kDriverLocalPos));
    Vector2 head = vehicleWorldToScreen(headWorld, context.camera);
    if (interp) {
        head.y += kBodyVisualYOffset;
    } else {
        head.x = std::round(head.x);
        head.y = std::round(head.y + kBodyVisualYOffset);
    }
    const b2Rot headRotInterp = b2NLerp(_headSnapshot.prevRot, _headSnapshot.curRot, alpha);
    const float headAngle =
        interp ? b2Rot_GetAngle(headRotInterp) * RAD2DEG
               : (driverHeadValid ? b2Rot_GetAngle(b2Body_GetRotation(_driverHeadId)) * RAD2DEG : chassisAngle);

    const DriverExpression expression = driverExpression(context.runSeed, context.gameOver, context.headHit);
    const Texture2D* driverTexture = &context.textures.driver;
    // Helmet always overrides the driver face when active.
    if (context.helmetActive) {
        if (textureLoaded(context.textures.driverHelmeted)) {
            driverTexture = &context.textures.driverHelmeted;
        }
    } else {
        switch (expression.face) {
            case DriverFace::Cried:
                if (textureLoaded(context.textures.driverCried)) {
                    driverTexture = &context.textures.driverCried;
                }
                break;
            case DriverFace::Shocked:
                if (textureLoaded(context.textures.driverShocked)) {
                    driverTexture = &context.textures.driverShocked;
                }
                break;
            case DriverFace::Scared:
                if (textureLoaded(context.textures.driverScared)) {
                    driverTexture = &context.textures.driverScared;
                }
                break;
            case DriverFace::Blink:
                if (textureLoaded(context.textures.driverBlink)) {
                    driverTexture = &context.textures.driverBlink;
                }
                break;
            case DriverFace::Normal:
            default:
                break;
        }
    }

    const bool hitFlash = expression.hitFlash;
    const Color driverTint = hitFlash ? Color{255, 150, 150, 255} : WHITE;
    if (textureLoaded(*driverTexture)) {
        drawSpriteCentered(*driverTexture, head, 10.0f, 10.0f, headAngle, driverTint);
    } else {
        DrawRectangle(static_cast<int>(head.x - 3.0f),
                      static_cast<int>(head.y - 3.0f),
                      6,
                      6,
                      hitFlash ? Color{230, 78, 68, 255} : Color{247, 210, 146, 255});
        DrawRectangleLines(
            static_cast<int>(head.x - 3.0f), static_cast<int>(head.y - 3.0f), 6, 6, Color{92, 52, 43, 255});
    }

    const float day = context.dayAmount;
    const float night = 1.0f - day;
    if (textureLoaded(context.textures.carBodyNight) || textureLoaded(context.textures.carBodyDay)) {
        if (textureLoaded(context.textures.carBodyNight) && night > 0.01f) {
            drawSpriteCentered(
                context.textures.carBodyNight, chassis, 31.0f, 13.0f, chassisAngle, fadeColor(WHITE, night));
        }
        if (textureLoaded(context.textures.carBodyDay) && day > 0.01f) {
            drawSpriteCentered(context.textures.carBodyDay, chassis, 31.0f, 13.0f, chassisAngle, fadeColor(WHITE, day));
        }
    } else {
        DrawRectanglePro(
            Rectangle{chassis.x, chassis.y, 32.0f, 10.0f}, Vector2{16.0f, 5.0f}, chassisAngle, Color{232, 92, 66, 255});
        DrawRectanglePro(Rectangle{chassis.x + 1.0f, chassis.y - 7.0f, 16.0f, 8.0f},
                         Vector2{8.0f, 4.0f},
                         chassisAngle,
                         Color{91, 166, 211, 255});
        DrawRectanglePro(Rectangle{chassis.x - 4.0f, chassis.y + 5.0f, 26.0f, 3.0f},
                         Vector2{13.0f, 1.5f},
                         chassisAngle,
                         Color{116, 42, 45, 255});
    }

    auto drawWheel = [&](b2BodyId wheelId, const BodySnapshot& snapshot) {
        if (!b2Body_IsValid(wheelId)) {
            return;
        }
        // Position interpolates between snapshots; rotation always reads live state
        // (wheels spin far more than half a turn per physics step, so interpolating
        // the angle would pick the wrong short-arc direction and look jittery).
        const b2Vec2 wheelPos = interp ? b2Lerp(snapshot.prevPos, snapshot.curPos, alpha) : b2Body_GetPosition(wheelId);
        Vector2 p = vehicleWorldToScreen(wheelPos, context.camera);
        if (!interp) {
            p.x = std::round(p.x);
            p.y = std::round(p.y);
        }
        const float angle = b2Rot_GetAngle(b2Body_GetRotation(wheelId));
        if (textureLoaded(context.textures.wheelNight) || textureLoaded(context.textures.wheelDay)) {
            if (textureLoaded(context.textures.wheelNight) && night > 0.01f) {
                drawSpriteCentered(
                    context.textures.wheelNight, p, 10.0f, 10.0f, angle * RAD2DEG, fadeColor(WHITE, night));
            }
            if (textureLoaded(context.textures.wheelDay) && day > 0.01f) {
                drawSpriteCentered(context.textures.wheelDay, p, 10.0f, 10.0f, angle * RAD2DEG, fadeColor(WHITE, day));
            }
        } else {
            DrawRectangle(static_cast<int>(p.x - 6.0f), static_cast<int>(p.y - 6.0f), 12, 12, Color{12, 13, 16, 255});
            DrawRectangle(static_cast<int>(p.x - 3.0f), static_cast<int>(p.y - 3.0f), 6, 6, Color{96, 100, 108, 255});
            DrawLineEx(
                p, {p.x + std::cos(angle) * 5.0f, p.y + std::sin(angle) * 5.0f}, 2.0f, Color{226, 228, 232, 255});
        }
    };

    drawWheel(_rearWheelId, _rearWheelSnapshot);
    drawWheel(_frontWheelId, _frontWheelSnapshot);
}

bool Vehicle::shapeBelongsToVehicle(b2ShapeId shapeId) const
{
    return b2Shape_IsValid(shapeId) && (B2_ID_EQUALS(shapeId, _chassisShape) || B2_ID_EQUALS(shapeId, _headShape) ||
                                        B2_ID_EQUALS(shapeId, _frontShape) || B2_ID_EQUALS(shapeId, _rearShape));
}

bool Vehicle::wheelGrounded(b2ShapeId shapeId) const
{
    if (!b2Shape_IsValid(shapeId)) {
        return false;
    }

    const int capacity = b2Shape_GetContactCapacity(shapeId);
    if (capacity <= 0) {
        return false;
    }

    std::vector<b2ContactData> contacts(static_cast<size_t>(capacity));
    const int count = b2Shape_GetContactData(shapeId, contacts.data(), capacity);
    for (int i = 0; i < count; ++i) {
        const b2ContactData& contact = contacts[static_cast<size_t>(i)];
        if (contact.manifold.pointCount <= 0) {
            continue;
        }

        b2ShapeId other = B2_ID_EQUALS(contact.shapeIdA, shapeId) ? contact.shapeIdB : contact.shapeIdA;
        if (shapeBelongsToVehicle(other)) {
            continue;
        }

        for (int point = 0; point < contact.manifold.pointCount; ++point) {
            const b2ManifoldPoint& manifoldPoint = contact.manifold.points[point];
            if (manifoldPoint.totalNormalImpulse > 0.0f || manifoldPoint.separation <= 0.035f) {
                return true;
            }
        }
    }

    return false;
}

void Vehicle::updateGroundState()
{
    _rearGrounded = wheelGrounded(_rearShape);
    _frontGrounded = wheelGrounded(_frontShape);
}

bool Vehicle::consumeLandingEvent(float& fallSpeed)
{
    if (!_landingEvent) {
        return false;
    }
    fallSpeed = _landingFallSpeed;
    _landingEvent = false;
    return true;
}

bool Vehicle::valid() const
{
    return b2Body_IsValid(_chassisId);
}

float Vehicle::distanceFrom(float startX) const
{
    if (!valid()) {
        return 0.0f;
    }
    const b2Vec2 pos = b2Body_GetPosition(_chassisId);
    return std::max(0.0f, pos.x - startX);
}

b2Vec2 Vehicle::chassisPosition() const
{
    return b2Body_IsValid(_chassisId) ? b2Body_GetPosition(_chassisId) : b2Vec2{};
}

b2Vec2 Vehicle::chassisVelocity() const
{
    return b2Body_IsValid(_chassisId) ? b2Body_GetLinearVelocity(_chassisId) : b2Vec2{};
}

float Vehicle::chassisAngularVelocity() const
{
    return b2Body_IsValid(_chassisId) ? b2Body_GetAngularVelocity(_chassisId) : 0.0f;
}

b2Rot Vehicle::chassisRotation() const
{
    return b2Body_IsValid(_chassisId) ? b2Body_GetRotation(_chassisId) : b2MakeRot(0.0f);
}

b2Vec2 Vehicle::chassisWorldPoint(b2Vec2 localPoint) const
{
    return b2Body_IsValid(_chassisId) ? b2Body_GetWorldPoint(_chassisId, localPoint) : localPoint;
}

b2Vec2 Vehicle::renderChassisWorldPoint(b2Vec2 localPoint, bool interp, float alpha) const
{
    if (interp && _hasSnapshot) {
        const b2Transform xf{b2Lerp(_chassisSnapshot.prevPos, _chassisSnapshot.curPos, alpha),
                             b2NLerp(_chassisSnapshot.prevRot, _chassisSnapshot.curRot, alpha)};
        return b2TransformPoint(xf, localPoint);
    }
    return chassisWorldPoint(localPoint);
}

float Vehicle::renderChassisAngleDeg(bool interp, float alpha) const
{
    const b2Rot rot = (interp && _hasSnapshot) ? b2NLerp(_chassisSnapshot.prevRot, _chassisSnapshot.curRot, alpha)
                                               : chassisRotation();
    return b2Rot_GetAngle(rot) * RAD2DEG;
}

float Vehicle::renderDistanceFrom(float startX, bool interp, float alpha) const
{
    if (interp && _hasSnapshot) {
        const float x = b2Lerp(_chassisSnapshot.prevPos, _chassisSnapshot.curPos, alpha).x;
        return std::max(0.0f, x - startX);
    }
    return distanceFrom(startX);
}

b2Vec2 Vehicle::frontWheelPosition() const
{
    return b2Body_IsValid(_frontWheelId) ? b2Body_GetPosition(_frontWheelId) : b2Vec2{};
}

b2Vec2 Vehicle::rearWheelPosition() const
{
    return b2Body_IsValid(_rearWheelId) ? b2Body_GetPosition(_rearWheelId) : b2Vec2{};
}

void Vehicle::applyChassisDeltaVelocity(b2Vec2 deltaVelocity)
{
    applyDeltaVelocityToBody(_chassisId, deltaVelocity, 1.0f);
}

void Vehicle::applyMainBodyDeltaVelocity(b2Vec2 deltaVelocity,
                                         float chassisScale,
                                         float frontWheelScale,
                                         float rearWheelScale)
{
    applyDeltaVelocityToBody(_chassisId, deltaVelocity, chassisScale);
    applyDeltaVelocityToBody(_frontWheelId, deltaVelocity, frontWheelScale);
    applyDeltaVelocityToBody(_rearWheelId, deltaVelocity, rearWheelScale);
}

void Vehicle::applyChassisForcePerMass(b2Vec2 forcePerMass)
{
    applyForcePerMassToBody(_chassisId, forcePerMass, 1.0f);
}

void Vehicle::applyMainBodyForcePerMass(b2Vec2 forcePerMass,
                                        float chassisScale,
                                        float frontWheelScale,
                                        float rearWheelScale)
{
    applyForcePerMassToBody(_chassisId, forcePerMass, chassisScale);
    applyForcePerMassToBody(_frontWheelId, forcePerMass, frontWheelScale);
    applyForcePerMassToBody(_rearWheelId, forcePerMass, rearWheelScale);
}

void Vehicle::applyChassisAngularImpulse(float impulse)
{
    if (b2Body_IsValid(_chassisId)) {
        b2Body_ApplyAngularImpulse(_chassisId, impulse, true);
    }
}

void Vehicle::applyChassisTorque(float torque)
{
    if (b2Body_IsValid(_chassisId)) {
        b2Body_ApplyTorque(_chassisId, torque, true);
    }
}

b2BodyId Vehicle::chassisId() const
{
    return _chassisId;
}

b2BodyId Vehicle::driverHeadId() const
{
    return _driverHeadId;
}

b2BodyId Vehicle::frontWheelId() const
{
    return _frontWheelId;
}

b2BodyId Vehicle::rearWheelId() const
{
    return _rearWheelId;
}

b2ShapeId Vehicle::chassisShape() const
{
    return _chassisShape;
}

b2ShapeId Vehicle::headShape() const
{
    return _headShape;
}

b2ShapeId Vehicle::frontShape() const
{
    return _frontShape;
}

b2ShapeId Vehicle::rearShape() const
{
    return _rearShape;
}

bool Vehicle::frontGrounded() const
{
    return _frontGrounded;
}

bool Vehicle::rearGrounded() const
{
    return _rearGrounded;
}

void RidgeDashGame::createVehicle()
{
    _vehicle.create(_worldId, _startX, _terrain.heightAt(_startX) - 1.7f);
}

void RidgeDashGame::processInput(float dt)
{
    const GameInput::Drive& drive = _input.drive();
    const bool canDrive = _runController.canDrive();

    if (_runController.startIfReady(drive.throttle || drive.brake)) {
        if (_ui.startTipsVisible()) {
            _ui.startTipsExit();
        }
    }

    if (!canDrive) {
        _vehicle.applyDriveInput(Vehicle::DriveInput{drive.throttle, drive.brake, false});
        return;
    }

    const Vehicle::DriveResult result =
        _vehicle.applyDriveInput(Vehicle::DriveInput{drive.throttle, drive.brake, true});
    if (result.fuelBurnScale > 0.0f) {
        _runController.burnFuel(kFuelBurnPerSecond * result.fuelBurnScale * dt);
    }
}

void RidgeDashGame::updateGroundState()
{
    _vehicle.updateGroundState();
}

void RidgeDashGame::updateDust(float dt)
{
    TerrainBiome rearBiome = TerrainBiome::Mountain;
    TerrainBiome frontBiome = TerrainBiome::Mountain;
    if (carValid()) {
        rearBiome = _terrain.biomeAt(_vehicle.rearWheelPosition().x);
        frontBiome = _terrain.biomeAt(_vehicle.frontWheelPosition().x);
    }
    _vehicle.updateDust(dt, _runController.running(), _runSeed, rearBiome, frontBiome);

    // Touchdown sound: heavy drop -> car_landing, light bump -> car_hit.
    float fallSpeed = 0.0f;
    if (_vehicle.consumeLandingEvent(fallSpeed) && fallSpeed > kLandingSoundMinSpeed) {
        playSfx(fallSpeed > kHardLandingSpeed ? AudioSystem::Sfx::CarLanding : AudioSystem::Sfx::CarHit);
    }
}

bool RidgeDashGame::carValid() const
{
    return _vehicle.valid();
}

float RidgeDashGame::carDistance() const
{
    if (!carValid()) {
        return _runStats.distance();
    }
    return _vehicle.distanceFrom(_startX);
}

bool RidgeDashGame::helmetActive() const
{
    return _helmetActive;
}

bool RidgeDashGame::magnetActive() const
{
    return _pickups.magnet().active();
}

void RidgeDashGame::drawDust() const
{
    _vehicle.drawDust(_camera);
}

void RidgeDashGame::drawVehicle() const
{
    Vehicle::DrawContext context{};
    context.textures.carBodyDay = _sprites.carBodyDay;
    context.textures.carBodyNight = _sprites.carBodyNight;
    context.textures.driver = _sprites.driver;
    context.textures.driverBlink = _sprites.driverBlink;
    context.textures.driverCried = _sprites.driverCried;
    context.textures.driverScared = _sprites.driverScared;
    context.textures.driverShocked = _sprites.driverShocked;
    context.textures.driverHelmeted = _sprites.driverHelmeted;
    context.textures.wheelDay = _sprites.wheelDay;
    context.textures.wheelNight = _sprites.wheelNight;
    context.camera = _camera;
    context.dayAmount = _environment.dayAmount(_runStats.distance());
    context.runSeed = _runSeed;
    context.gameOver = _runController.gameOver();
    context.headHit = _runController.headHit();
    context.helmetActive = _helmetActive;
    context.interpolate = _interpolate;
    context.alpha = _interpolate ? clampf(_physicsRemainder / kPhysicsStep, 0.0f, 1.0f) : 1.0f;
    _vehicle.draw(context);
}

} // namespace ridge_dash

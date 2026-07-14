/**
 * @file ridge_dash_game.cpp
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

#include <algorithm>
#include <cmath>
#include <ctime>

namespace ridge_dash {

using namespace game_config;

RidgeDashGame::RidgeDashGame()
{
    _runSeed = static_cast<uint32_t>(std::time(nullptr));
    loadSprites();
    _audio.load();
    _ui.configureAnimations();
    reset();
}

RidgeDashGame::~RidgeDashGame()
{
    destroyWorld();
    _audio.unload();
    unloadSprites();
}

void RidgeDashGame::loadSprites()
{
    _sprites.carBodyDay = loadSpriteTexture("car_body_day.png");
    _sprites.carBodyNight = loadSpriteTexture("car_body_night.png");
    _sprites.driver = loadSpriteTexture("driver.png");
    _sprites.driverBlink = loadSpriteTexture("driver_blink.png");
    _sprites.driverCried = loadSpriteTexture("driver_cried.png");
    _sprites.driverScared = loadSpriteTexture("driver_scared.png");
    _sprites.driverShocked = loadSpriteTexture("driver_shocked.png");
    _sprites.wheelDay = loadSpriteTexture("wheel_day.png");
    _sprites.wheelNight = loadSpriteTexture("wheel_night.png");
    _sprites.fuelCan = loadSpriteTexture("fuel_can.png");
    _sprites.coin = loadSpriteTexture("coin.png");
    _sprites.flea = loadSpriteTexture("flea.png");
    _sprites.rocket = loadSpriteTexture("rocket.png");
    _sprites.cactus = loadSpriteTexture("cactus.png");
    _sprites.snowman = loadSpriteTexture("snowman.png");
    _sprites.giantFlea = loadSpriteTexture("gaint_flea.png");
    _sprites.helmet = loadSpriteTexture("helmet.png");
    _sprites.driverHelmeted = loadSpriteTexture("driver_helmeted.png");
    _sprites.squidA = loadSpriteTexture("squid_a.png");
    _sprites.squidB = loadSpriteTexture("squid_b.png");
    _sprites.squidC = loadSpriteTexture("squid_c.png");
    _sprites.squidD = loadSpriteTexture("squid_d.png");
    _environment.loadAssets();
}

void RidgeDashGame::unloadSprites()
{
    unloadSpriteTexture(_sprites.carBodyDay);
    unloadSpriteTexture(_sprites.carBodyNight);
    unloadSpriteTexture(_sprites.driver);
    unloadSpriteTexture(_sprites.driverBlink);
    unloadSpriteTexture(_sprites.driverCried);
    unloadSpriteTexture(_sprites.driverScared);
    unloadSpriteTexture(_sprites.driverShocked);
    unloadSpriteTexture(_sprites.wheelDay);
    unloadSpriteTexture(_sprites.wheelNight);
    unloadSpriteTexture(_sprites.fuelCan);
    unloadSpriteTexture(_sprites.coin);
    unloadSpriteTexture(_sprites.flea);
    unloadSpriteTexture(_sprites.rocket);
    unloadSpriteTexture(_sprites.cactus);
    unloadSpriteTexture(_sprites.snowman);
    unloadSpriteTexture(_sprites.giantFlea);
    unloadSpriteTexture(_sprites.helmet);
    unloadSpriteTexture(_sprites.driverHelmeted);
    unloadSpriteTexture(_sprites.squidA);
    unloadSpriteTexture(_sprites.squidB);
    unloadSpriteTexture(_sprites.squidC);
    unloadSpriteTexture(_sprites.squidD);
    _environment.unloadAssets();
}

void RidgeDashGame::reset()
{
    destroyWorld();
    const uint32_t seed = _runSeed++;
    _rng.seed(seed);
    _environment.reset(seed);
    _runController.reset();
    _physicsRemainder = 0.0f;
    _runStats.reset();
    _trickTracker.reset();
    _ui.resetRun();
    _audio.resetEngine();
    _audio.startBgm();
    _bgmIntense = false;
    _bgmCalmTimer = 0.0f;
    _helmetActive = false;
    _helmetRescuedThisFrame = false;
    _invincibleTimer = 0.0f;
    _camera = {0.0f, 0.0f};
    _startX = 4.0f;
    createWorld();
    _terrain.reset(_startX, _worldId, _rng);
    createVehicle();
    _pickups.reset(*this);
    updateCamera(kPhysicsStep);
}

bool RidgeDashGame::shouldQuit() const
{
    return _quitRequested;
}

void RidgeDashGame::setDisplayScaleOption(DisplayScaleOption option)
{
    _pauseMenu.setDisplayScaleOption(option);
}

RidgeDashGame::DisplayScaleOption RidgeDashGame::displayScaleOption() const
{
    return _pauseMenu.displayScaleOption();
}

bool RidgeDashGame::consumeDisplayScaleRequest(DisplayScaleOption& option)
{
    return _pauseMenu.consumeDisplayScaleRequest(option);
}

void RidgeDashGame::setCrtEnabled(bool enabled)
{
    _pauseMenu.setCrtEnabled(enabled);
}

bool RidgeDashGame::crtEnabled() const
{
    return _pauseMenu.crtEnabled();
}

bool RidgeDashGame::consumeCrtRequest(bool& enabled)
{
    return _pauseMenu.consumeCrtRequest(enabled);
}

void RidgeDashGame::setInterpolationEnabled(bool enabled)
{
    _interpolate = enabled;
}

bool RidgeDashGame::renderInterpolation() const
{
    return _interpolate;
}

float RidgeDashGame::renderAlpha() const
{
    return _interpolate ? clampf(_physicsRemainder / kPhysicsStep, 0.0f, 1.0f) : 1.0f;
}

void RidgeDashGame::playSfx(AudioSystem::Sfx id)
{
    _audio.play(id);
}

void RidgeDashGame::destroyWorld()
{
    if (b2World_IsValid(_worldId)) {
        b2DestroyWorld(_worldId);
    }
    _worldId = b2_nullWorldId;
    _vehicle.reset();
    _terrain.clearBodies();
    _pickups.clear();
}

void RidgeDashGame::createWorld()
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 16.5f};
    _worldId = b2CreateWorld(&worldDef);
    b2World_SetContactTuning(_worldId, 110.0f, 0.56f, 8.0f);
}

void RidgeDashGame::update(float dt)
{
    dt = std::min(dt, kMaxFrameDt);
    _input.poll();

    // Keep the engine stream fed every frame (including while paused/game over) so
    // its buffer never starves; the state flags handle muting.
    updateEngineAudio(dt);
    updateBgmAudio(dt);

    if (_runController.paused()) {
        updatePauseMenu(dt);
        _ui.updateAnimations(dt, false, _runController.gameOverTimer());
        return;
    }

    if (_input.commands().pause && !_runController.gameOver()) {
        enterPauseMenu();
        _ui.updateAnimations(dt, false, _runController.gameOverTimer());
        return;
    }

    if (_input.commands().reset || (_runController.gameOver() && _input.commands().confirm)) {
        submitRunRecord();
        reset();
        return;
    }

    if (_input.commands().squid) {
        _pickups.triggerSquid(_runSeed);
    }

    _runController.updateFrameTimers(dt);
    _invincibleTimer = std::max(0.0f, _invincibleTimer - dt);
    updateWorldStreaming();
    updateGroundState();
    processInput(dt);
    _pickups.update(*this, dt);
    stepPhysics(dt);
    _helmetRescuedThisFrame = false;
    handleSensorEvents();
    updatePickupOverlaps();
    updateGroundState();
    updateDriverExpression(dt);
    updateTricks(dt);
    updateDust(dt);
    _ui.updateAnimations(dt, _runController.gameOver(), _runController.gameOverTimer());
    _ui.updateScorePopup(dt);
    _pickups.updateEffects(dt);
    updateRunState(dt);
    updateCamera(dt);
    updateEnvironmentBiome(dt);
}

void RidgeDashGame::updateWorldStreaming()
{
    if (!carValid()) {
        return;
    }

    const float carX = _vehicle.chassisPosition().x;
    _terrain.appendUntil(carX + kTerrainGenerateAhead, _rng);
    _pickups.stream(*this, carX);
    _pickups.trim(carX - kTerrainKeepBehind);
    _terrain.trimBehind(carX - kTerrainKeepBehind);
}

void RidgeDashGame::stepPhysics(float dt)
{
    if (!b2World_IsValid(_worldId)) {
        return;
    }

    _physicsRemainder += dt;
    int steps = 0;
    while (_physicsRemainder >= kPhysicsStep && steps < 4) {
        // Apply continuous forces once per fixed step so their net impulse is
        // framerate-independent (see Vehicle/Rocket/Snowman applyStepForces).
        _vehicle.applyStepForces();
        _pickups.applyStepForces(*this);
        b2World_Step(_worldId, kPhysicsStep, 8);
        _physicsRemainder -= kPhysicsStep;
        ++steps;
        if (_interpolate) {
            _vehicle.recordPhysicsSnapshot();
        }
    }
    if (steps == 4) {
        _physicsRemainder = 0.0f;
    }
}

void RidgeDashGame::updateEnvironmentBiome(float dt)
{
    const float sampleX = carValid() ? _vehicle.chassisPosition().x + 10.0f : _startX;
    _environment.updateBiome(_terrain.biomeAt(sampleX), dt);
}

void RidgeDashGame::updateEngineAudio(float dt)
{
    const GameInput::Drive& drive = _input.drive();
    AudioSystem::EngineState state{};
    // Total speed (includes vertical) so launches/falls/landings shift the note.
    state.speed = carValid() ? length(_vehicle.chassisVelocity()) : 0.0f;
    state.throttle = drive.throttle;
    state.brake = drive.brake;
    // Idle audible from game start (menu/ready + running); muted on pause/game over.
    state.on = !_runController.paused() && !_runController.gameOver();
    _audio.updateEngine(dt, state);
}

void RidgeDashGame::updateBgmAudio(float dt)
{
    const float speed = carValid() ? length(_vehicle.chassisVelocity()) : 0.0f;

    // Calm <-> intense state machine with hysteresis + a dwell before calming down,
    // so a quick dip in speed doesn't flip the music back and forth. Only escalates
    // while actually running; menu/game-over target calm.
    if (_runController.running()) {
        if (!_bgmIntense) {
            if (speed > kBgmIntenseSpeed) {
                _bgmIntense = true;
                _bgmCalmTimer = 0.0f;
            }
        } else if (speed < kBgmCalmSpeed) {
            _bgmCalmTimer += dt;
            if (_bgmCalmTimer > kBgmCalmDwell) {
                _bgmIntense = false;
                _bgmCalmTimer = 0.0f;
            }
        } else {
            _bgmCalmTimer = 0.0f;
        }
    } else {
        _bgmIntense = false;
        _bgmCalmTimer = 0.0f;
    }

    AudioSystem::BgmState state{};
    state.intense = _bgmIntense;
    state.audible = !_runController.paused();  // paused -> duck
    state.active = !_runController.gameOver(); // game over -> fade out
    _audio.updateBgm(dt, state);
}

void RidgeDashGame::updateDriverExpression(float dt)
{
    const float groundY = carValid() ? _terrain.heightAt(_vehicle.chassisPosition().x) : 0.0f;
    _vehicle.updateDriverExpression(dt, groundY, _runController.gameOver());
}

void RidgeDashGame::showScorePopup(int amount, const char* label)
{
    _ui.showScorePopup(amount, label);
}

void RidgeDashGame::updateTricks(float dt)
{
    if (!carValid() || _runController.gameOver()) {
        _trickTracker.reset();
        return;
    }

    const TrickTracker::Bonus bonus = _trickTracker.update(dt,
                                                           TrickTracker::Sample{
                                                               b2Rot_GetAngle(_vehicle.chassisRotation()),
                                                               std::abs(_vehicle.chassisAngularVelocity()),
                                                               _vehicle.frontGrounded(),
                                                               _vehicle.rearGrounded(),
                                                               // Not blocked by a head hit: like distance, flips keep
                                                               // counting after touchdown until the run truly ends
                                                               // (updateTricks already early-outs on gameOver()).
                                                               false,
                                                           });

    // Each flip is credited the instant it completes, mid-air: score, an updated
    // "Nx FLIP" popup, and the flip sound all fire immediately and keep counting.
    if (bonus.newFlip) {
        const int score = bonus.frontFlip ? kFrontFlipBonusScore : kFlipBonusScore;
        const char* dirLabel = bonus.frontFlip ? "FLIP" : "FLIP";
        _runStats.addFlipBonus(bonus.frontFlip, score);
        showScorePopup(score, bonus.flipIndex > 1 ? TextFormat("%dx %s", bonus.flipIndex, dirLabel) : dirLabel);
        playSfx(AudioSystem::Sfx::CarFlip);
    }
}

void RidgeDashGame::updatePauseMenu(float dt)
{
    _ui.updatePauseTimer(dt);

    // Click on any menu navigation / confirm / back key (all edge-triggered).
    const GameInput::Menu& menu = _input.menu();
    if (menu.up || menu.down || menu.left || menu.right || menu.confirm || menu.back) {
        playSfx(AudioSystem::Sfx::UiSelect);
    }

    switch (_pauseMenu.update(_input.menu(), _ui)) {
        case PauseMenuController::Action::ExitPause:
            exitPauseMenu();
            break;
        case PauseMenuController::Action::QuitGame:
            requestGameExit();
            break;
        case PauseMenuController::Action::None:
            break;
    }

    if (_ui.pauseExitFinished()) {
        _runController.completePauseExit();
        _ui.completePauseExit();
    }
}

void RidgeDashGame::enterPauseMenu()
{
    if (!_runController.enterPause()) {
        return;
    }
    _ui.enterPause();
    playSfx(AudioSystem::Sfx::UiSelect);
}

void RidgeDashGame::exitPauseMenu()
{
    if (_ui.pauseExiting()) {
        return;
    }
    _ui.startPauseExit();
}

void RidgeDashGame::requestGameExit()
{
    submitRunRecord();
    _quitRequested = true;
}

void RidgeDashGame::submitRunRecord()
{
    _runRecords.submit(_runStats);
}

void RidgeDashGame::updateRunState(float dt)
{
    if (!carValid()) {
        return;
    }

    // Lock the score once the run ends so slow post-crash creep can't inflate it.
    if (!_runController.gameOver()) {
        _runStats.updateDistance(carDistance());
    }

    const b2Vec2 pos = _vehicle.chassisPosition();
    const b2Vec2 vel = _vehicle.chassisVelocity();
    const b2Rot rotation = _vehicle.chassisRotation();
    const float angle = std::abs(std::atan2(rotation.s, rotation.c));
    const float angularSpeed = std::abs(_vehicle.chassisAngularVelocity());

    if (_runController.updateEndConditions(dt, RunController::MotionSample{pos, vel, angle, angularSpeed})) {
        submitRunRecord();
    }
}

void RidgeDashGame::updateCamera(float dt)
{
    if (!carValid()) {
        return;
    }

    // Convert per-physics-step lerp coefficients into framerate-independent factors
    // so the camera converges at the same rate regardless of render frame rate.
    // factor = 1 - (1 - k)^(dt / kPhysicsStep) reproduces the old behaviour exactly
    // when dt == kPhysicsStep.
    const float stepRatio = dt / kPhysicsStep;
    auto frameFactor = [stepRatio](float k) {
        return 1.0f - std::pow(1.0f - k, stepRatio);
    };

    const b2Vec2 pos = _vehicle.chassisPosition();
    const b2Vec2 vel = _vehicle.chassisVelocity();
    const float groundY = _terrain.heightAt(pos.x);
    const float heightAboveGround = std::max(0.0f, groundY - pos.y);
    const float airView = clampf((heightAboveGround - 2.6f) / 5.8f, 0.0f, 1.0f);
    const float targetScreenY = 88.0f - airView * 28.0f;
    const float verticalLookahead = clampf(vel.y * 3.4f, -34.0f, 18.0f);
    const float targetX = pos.x * kPixelsPerMeter - kScreenWidth * 0.34f;
    const float targetY = pos.y * kPixelsPerMeter - targetScreenY + verticalLookahead;
    const float currentScreenY = pos.y * kPixelsPerMeter - _camera.y;
    const float yFollow = currentScreenY < 26.0f ? 0.28f : (airView > 0.0f ? 0.16f : 0.10f);
    _camera.x += (std::max(0.0f, targetX) - _camera.x) * frameFactor(0.12f);
    _camera.y += (clampf(targetY, -170.0f, 70.0f) - _camera.y) * frameFactor(yFollow);

    // Safety clamp: never let the car leave the top of the screen on a fast, high
    // launch. The smooth follow above can lag or hit its target clamp; this keeps
    // the car at least kMinCarScreenY below the top edge regardless of speed.
    constexpr float kMinCarScreenY = 14.0f;
    const float carScreenY = pos.y * kPixelsPerMeter - _camera.y;
    if (carScreenY < kMinCarScreenY) {
        _camera.y = pos.y * kPixelsPerMeter - kMinCarScreenY;
    }
}

} // namespace ridge_dash

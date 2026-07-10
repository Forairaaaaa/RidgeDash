/**
 * @file run_controller.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/run/run_controller.hpp"

#include "game/game_config.hpp"

#include <algorithm>

namespace ridge_dash {
namespace {

constexpr float kFinishCrashLinearSpeed = 0.42f;
constexpr float kFinishCrashAngularSpeed = 0.55f;
constexpr float kFinishCrashDelay = 0.6f;
constexpr float kFinishCrashBackwardSpeed = -0.12f;
constexpr float kFinishCrashBackwardDelay = 0.2f;

} // namespace

using namespace game_config;

void RunController::reset()
{
    _state = RunState::Ready;
    _stateBeforePause = RunState::Ready;
    _fuel = kMaxFuel;
    _fuelFlash = 0.0f;
    _stallTimer = 0.0f;
    _crashStallTimer = 0.0f;
    _finishCrashTimer = 0.0f;
    _gameOverTimer = 0.0f;
    _headHit = false;
}

void RunController::updateFrameTimers(float dt)
{
    if (_state == RunState::GameOver) {
        _gameOverTimer += dt;
    }
    _fuelFlash = std::max(0.0f, _fuelFlash - dt);
}

bool RunController::startIfReady(bool inputActive)
{
    if (_state != RunState::Ready || !inputActive) {
        return false;
    }

    _state = RunState::Running;
    return true;
}

bool RunController::enterPause()
{
    if (_state == RunState::Paused || _state == RunState::GameOver) {
        return false;
    }

    _stateBeforePause = _state;
    _state = RunState::Paused;
    return true;
}

void RunController::completePauseExit()
{
    _state = _stateBeforePause;
}

bool RunController::setGameOver()
{
    if (_state == RunState::GameOver) {
        return false;
    }

    _state = RunState::GameOver;
    return true;
}

bool RunController::updateEndConditions(float dt, const MotionSample& sample)
{
    const float speed = length(sample.velocity);

    if (sample.position.y > 18.5f) {
        return setGameOver();
    }

    const bool upsideDown = sample.angleAbs > 1.85f;
    const bool crashPending = _headHit || (_state == RunState::Running && upsideDown);
    const bool mostlyStopped = speed < kFinishCrashLinearSpeed && sample.angularSpeed < kFinishCrashAngularSpeed;
    // Rolling backward means the car has lost its forward run: end it fast
    // instead of waiting out several oscillations in a small dip.
    const bool rollingBackward = sample.velocity.x < kFinishCrashBackwardSpeed;

    if (crashPending && (mostlyStopped || rollingBackward)) {
        _finishCrashTimer += dt;
        const float delay = rollingBackward ? kFinishCrashBackwardDelay : kFinishCrashDelay;
        if (_finishCrashTimer > delay) {
            return setGameOver();
        }
    } else {
        _finishCrashTimer = 0.0f;
    }

    if (_state == RunState::Running && upsideDown && speed < 0.38f) {
        _crashStallTimer += dt;
    } else {
        _crashStallTimer = 0.0f;
    }

    if (_fuel <= 0.0f && (speed < 0.25f || rollingBackward)) {
        _stallTimer += dt;
        if (_stallTimer > 1.4f) {
            return setGameOver();
        }
    } else {
        _stallTimer = 0.0f;
    }

    return false;
}

void RunController::burnFuel(float amount)
{
    _fuel = std::max(0.0f, _fuel - amount);
}

void RunController::refillFuel()
{
    _fuel = kMaxFuel;
    _fuelFlash = 0.24f;
}

bool RunController::markHeadHit()
{
    if (_headHit) {
        return false;
    }

    _headHit = true;
    return true;
}

bool RunController::running() const
{
    return _state == RunState::Running;
}

bool RunController::paused() const
{
    return _state == RunState::Paused;
}

bool RunController::gameOver() const
{
    return _state == RunState::GameOver;
}

bool RunController::canDrive() const
{
    return _state != RunState::GameOver && !_headHit && _fuel > 0.0f;
}

bool RunController::headHit() const
{
    return _headHit;
}

float RunController::fuel() const
{
    return _fuel;
}

float RunController::fuelFlash() const
{
    return _fuelFlash;
}

float RunController::gameOverTimer() const
{
    return _gameOverTimer;
}

} // namespace ridge_dash

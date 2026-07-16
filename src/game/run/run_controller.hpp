/**
 * @file run_controller.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <box2d/box2d.h>

namespace ridge_dash {

enum class RunState { Ready, Running, Paused, GameOver };

class RunController {
public:
    struct MotionSample {
        b2Vec2 position{};
        b2Vec2 velocity{};
        float angleAbs = 0.0f;
        float angularSpeed = 0.0f;
    };

    void reset();
    void updateFrameTimers(float dt);
    bool startIfReady(bool inputActive);
    bool enterPause();
    void completePauseExit();
    bool setGameOver();
    bool updateEndConditions(float dt, const MotionSample& sample);

    void burnFuel(float amount);
    void refillFuel();
    bool markHeadHit();
    void resetCrashTimer();

    bool running() const;
    bool paused() const;
    bool gameOver() const;
    bool canDrive() const;
    bool headHit() const;
    float fuel() const;
    float fuelFlash() const;
    float gameOverTimer() const;

private:
    RunState _state = RunState::Ready;
    RunState _stateBeforePause = RunState::Ready;
    float _fuel = 100.0f;
    float _fuelFlash = 0.0f;
    float _stallTimer = 0.0f;
    float _crashStallTimer = 0.0f;
    float _finishCrashTimer = 0.0f;
    float _gameOverTimer = 0.0f;
    bool _headHit = false;
};

} // namespace ridge_dash

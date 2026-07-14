/**
 * @file trick_tracker.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

namespace ridge_dash {

class TrickTracker {
public:
    struct Sample {
        float angle = 0.0f;
        float angularSpeed = 0.0f;
        bool frontGrounded = false;
        bool rearGrounded = false;
        bool blocked = false;
    };

    struct Bonus {
        bool newFlip = false;  // a flip just completed this frame (score + popup + sfx)
        int flipIndex = 0;     // cumulative flip number this air session (1,2,3,...) when newFlip
        bool frontFlip = true; // true = front flip (nose-down rotation), false = back flip
    };

    void reset();
    Bonus update(float dt, const Sample& sample);

private:
    float _airTime = 0.0f;
    float _flipAngle = 0.0f;
    float _lastFlipAngle = 0.0f;
    int _announcedFlips = 0;
    bool _wasAirborne = false;
};

} // namespace ridge_dash

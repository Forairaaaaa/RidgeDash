/**
 * @file audio_system.hpp
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

#if defined(RIDGEDASH_ENABLE_AUDIO)
#include <random>
#include <string>
#include <vector>
#endif

namespace ridge_dash {

// Owns game audio. Real playback is compiled only when RIDGEDASH_ENABLE_AUDIO is
// defined (Desktop); on DRM/FBDEV every method is a no-op so the rest of the game
// can call it unconditionally.
class AudioSystem {
public:
    // One-shot event sounds. Groups with several variants pick one at random.
    enum class Sfx {
        Coin,         // coin_pickup_1/2
        Fuel,         // fuel_can_pickup
        Cactus,       // cactus_pickup
        Snowman,      // snowman_pickup
        Rocket,       // rocket_pick
        RocketFinish, // rocket_finish (flight-end explosion)
        FleaJump,     // flea_jump
        CarLanding,   // car_landing_1/2/3  (hard, high-fall touchdown)
        CarHit,       // car_hit_1/2/3      (light landing / small bump)
        CarFlip,      // car_flip           (each mid-air flip)
        UiSelect,     // ui_select          (menu navigation / confirm)
        Count
    };

    // Per-frame engine state used to shape the looping engine sound.
    struct EngineState {
        float speed = 0.0f;    // |chassis velocity| in m/s (total, includes vertical)
        bool throttle = false; // throttle input held
        bool brake = false;    // brake/reverse input held
        bool on = false;       // engine running: idles from game start, muted on pause/game-over
    };

    // Per-frame music state. Two looping tracks (calm + intense) crossfade based on
    // `intense`; `audible` ducks on pause; `active` fades out + is silent on game over.
    struct BgmState {
        bool intense = false;
        bool audible = true;
        bool active = true;
    };

    AudioSystem() = default;
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    void load();
    void unload();
    void resetEngine();
    void updateEngine(float dt, const EngineState& state);
    void play(Sfx id);
    void startBgm(); // (re)pick random calm + intense tracks and begin playback
    void updateBgm(float dt, const BgmState& state);

    void setBgmMuted(bool muted);
    bool bgmMuted() const;
    void setSfxMuted(bool muted);
    bool sfxMuted() const;

private:
#if defined(RIDGEDASH_ENABLE_AUDIO)
    void loadSfxGroup(Sfx id, std::initializer_list<const char*> files);
    void stopBgm();
    // Next track from a category's shuffle bag: plays every track once (in a random
    // order) before any repeats. `lastPick` avoids repeating across a bag refill.
    std::string nextBgmTrack(const std::vector<std::string>& paths,
                             std::vector<std::size_t>& bag,
                             std::size_t& lastPick);

    Music _engine{};
    bool _engineLoaded = false;
    float _level = 0.0f;  // smoothed 0..1 engine drive level -> pitch
    float _volume = 0.0f; // smoothed 0..1 output volume

    std::vector<Sound> _sfx[static_cast<int>(Sfx::Count)];

    std::vector<std::string> _calmPaths;        // discovered files in music/calm/
    std::vector<std::string> _intensePaths;     // discovered files in music/intense/
    std::vector<std::size_t> _calmBag;          // remaining shuffled indices for calm
    std::vector<std::size_t> _intenseBag;       // remaining shuffled indices for intense
    std::size_t _calmLast = ~std::size_t{0};    // last calm index drawn (no back-to-back)
    std::size_t _intenseLast = ~std::size_t{0}; // last intense index drawn
    Music _bgmCalm{};
    Music _bgmIntense{};
    bool _bgmCalmLoaded = false;
    bool _bgmIntenseLoaded = false;
    // Active-track model: the track for the current state plays from its start at full
    // volume (no fade-in); the other fades out. -1 = none playing yet (delay window).
    int _bgmActive = -1;          // -1 none, 0 calm, 1 intense
    float _bgmCalmFade = 0.0f;    // 0..1 output level for the calm track
    float _bgmIntenseFade = 0.0f; // 0..1 output level for the intense track
    float _bgmMaster = 0.0f;      // overall bgm volume envelope (duck / fade-out)
    float _bgmCalmDelay = 0.0f;   // hold calm silent for this long after a reset

    bool _bgmMuted = false;
    bool _sfxMuted = false;
    std::mt19937 _rng{0xA17D};
#endif
};

} // namespace ridge_dash

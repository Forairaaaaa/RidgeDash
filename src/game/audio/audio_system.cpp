/**
 * @file audio_system.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/audio/audio_system.hpp"

#if defined(RIDGEDASH_ENABLE_AUDIO)

#include "game/game_config.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <string>

namespace ridge_dash {
namespace {

using game_config::clampf;
using game_config::kPhysicsStep;

// Engine sound shaping.
constexpr float kIdlePitch = 0.74f;       // playback pitch at idle
constexpr float kMaxPitch = 1.92f;        // playback pitch at full rev
constexpr float kSpeedForMax = 24.0f;     // total speed (m/s) mapping to full rev
constexpr float kSpeedWeight = 0.82f;     // how much actual speed drives the level (env feedback)
constexpr float kThrottleBump = 0.30f;    // throttle punch added on top of speed
constexpr float kBrakeBump = 0.12f;       // brake/reverse revs a little too
constexpr float kIdleVolume = 0.50f;      // volume while idling/coasting
constexpr float kFullVolume = 1.2f;       // volume at full drive
constexpr float kLevelSmoothing = 0.16f;  // per-60Hz-frame lerp toward target level
constexpr float kVolumeSmoothing = 0.20f; // per-60Hz-frame lerp toward target volume

// Music (BGM) shaping.
constexpr float kBgmVolume = 1.00f;          // master ceiling for background music
constexpr float kBgmPauseDuck = 0.30f;       // volume multiplier while paused
constexpr float kBgmCrossfadeTime = 1.3f;    // seconds to cross between calm and intense
constexpr float kBgmMasterSmoothing = 0.10f; // per-60Hz-frame lerp for the master envelope
constexpr float kBgmCalmResumeDelay = 5.0f;  // hold calm silent after a reset (skip the
                                             // calm->intense flash when relaunching fast)

// Framerate-independent lerp factor (matches the camera/engine smoothing elsewhere).
float frameFactor(float k, float dt)
{
    return 1.0f - std::pow(1.0f - k, dt / kPhysicsStep);
}

// Candidate on-disk locations for an audio file relative to assets/audio/.
std::array<std::string, 6> audioCandidates(const std::string& rel)
{
    const std::string appDir = GetApplicationDirectory();
    return {
        "assets/audio/" + rel,
        appDir + "assets/audio/" + rel,
        appDir + "../assets/audio/" + rel,
        appDir + "../Resources/assets/audio/" + rel, // macOS .app bundle
        appDir + "../share/ridgedash/audio/" + rel,
        "/usr/share/APPLaunch/share/ridgedash/audio/" + rel,
    };
}

// Collect audio track paths from music/<subdir>/ (e.g. "calm" or "intense"),
// resolving the directory across the usual candidate locations. Accepts .mp3 and
// .wav. Returns full paths ready to load.
std::vector<std::string> discoverBgm(const char* subdir)
{
    std::vector<std::string> paths;
    for (const std::string& dir : audioCandidates(std::string("music/") + subdir)) {
        if (!DirectoryExists(dir.c_str())) {
            continue;
        }
        FilePathList files = LoadDirectoryFilesEx(dir.c_str(), ".mp3;.wav", false);
        for (unsigned int i = 0; i < files.count; ++i) {
            paths.emplace_back(files.paths[i]);
        }
        UnloadDirectoryFiles(files);
        break; // first existing directory wins
    }
    return paths;
}

Music loadEngineMusic()
{
    for (const std::string& path : audioCandidates("engine_loop.wav")) {
        if (!FileExists(path.c_str())) {
            continue;
        }
        Music music = LoadMusicStream(path.c_str());
        if (IsMusicValid(music)) {
            return music;
        }
    }
    return Music{};
}

// Resolve an sfx file the same way sprites/music are resolved, then load it.
Sound loadSfx(const char* fileName)
{
    for (const std::string& path : audioCandidates(std::string("sfx/") + fileName)) {
        if (!FileExists(path.c_str())) {
            continue;
        }
        Sound sound = LoadSound(path.c_str());
        if (IsSoundValid(sound)) {
            return sound;
        }
    }
    return Sound{};
}

} // namespace

AudioSystem::~AudioSystem()
{
    unload();
}

void AudioSystem::loadSfxGroup(Sfx id, std::initializer_list<const char*> files)
{
    std::vector<Sound>& group = _sfx[static_cast<int>(id)];
    for (const char* file : files) {
        Sound sound = loadSfx(file);
        if (IsSoundValid(sound)) {
            group.push_back(sound);
        }
    }
}

void AudioSystem::load()
{
    if (!IsAudioDeviceReady()) {
        return;
    }

    if (!_engineLoaded) {
        _engine = loadEngineMusic();
        if (IsMusicValid(_engine)) {
            _engine.looping = true;
            _engineLoaded = true;
            _level = 0.0f;
            _volume = 0.0f;
            SetMusicVolume(_engine, 0.0f);
            SetMusicPitch(_engine, kIdlePitch);
            PlayMusicStream(_engine);
        }
    }

    loadSfxGroup(Sfx::Coin, {"coin_pickup_1.wav", "coin_pickup_2.wav"});
    loadSfxGroup(Sfx::Fuel, {"fuel_can_pickup.wav"});
    loadSfxGroup(Sfx::Cactus, {"cactus_pickup.wav"});
    loadSfxGroup(Sfx::Snowman, {"snowman_pickup.wav"});
    loadSfxGroup(Sfx::Rocket, {"rocket_pick.wav"});
    loadSfxGroup(Sfx::RocketFinish, {"rocket_finish.wav"});
    loadSfxGroup(Sfx::FleaJump, {"flea_jump.wav"});
    loadSfxGroup(Sfx::CarLanding, {"car_landing_1.wav", "car_landing_2.wav", "car_landing_3.wav"});
    loadSfxGroup(Sfx::CarHit, {"car_hit_1.wav", "car_hit_2.wav", "car_hit_3.wav"});
    loadSfxGroup(Sfx::CarFlip, {"car_flip.wav"});
    loadSfxGroup(Sfx::UiSelect, {"ui_select.wav"});

    // Discover available BGM variants by scanning music/calm/ and music/intense/.
    _calmPaths = discoverBgm("calm");
    _intensePaths = discoverBgm("intense");
}

void AudioSystem::unload()
{
    stopBgm();
    if (_engineLoaded) {
        StopMusicStream(_engine);
        UnloadMusicStream(_engine);
        _engine = Music{};
        _engineLoaded = false;
    }
    for (std::vector<Sound>& group : _sfx) {
        for (Sound& sound : group) {
            UnloadSound(sound);
        }
        group.clear();
    }
}

void AudioSystem::play(Sfx id)
{
    if (_sfxMuted) {
        return;
    }
    std::vector<Sound>& group = _sfx[static_cast<int>(id)];
    if (group.empty()) {
        return;
    }
    const std::size_t index =
        group.size() == 1 ? 0 : std::uniform_int_distribution<std::size_t>(0, group.size() - 1)(_rng);
    PlaySound(group[index]);
}

void AudioSystem::stopBgm()
{
    if (_bgmCalmLoaded) {
        StopMusicStream(_bgmCalm);
        UnloadMusicStream(_bgmCalm);
        _bgmCalm = Music{};
        _bgmCalmLoaded = false;
    }
    if (_bgmIntenseLoaded) {
        StopMusicStream(_bgmIntense);
        UnloadMusicStream(_bgmIntense);
        _bgmIntense = Music{};
        _bgmIntenseLoaded = false;
    }
}

std::string AudioSystem::nextBgmTrack(const std::vector<std::string>& paths,
                                      std::vector<std::size_t>& bag,
                                      std::size_t& lastPick)
{
    if (paths.empty()) {
        return {};
    }
    if (bag.empty()) {
        // Refill and shuffle: a fresh permutation of all indices.
        bag.resize(paths.size());
        for (std::size_t i = 0; i < paths.size(); ++i) {
            bag[i] = i;
        }
        std::shuffle(bag.begin(), bag.end(), _rng);
        // Avoid an immediate repeat of the just-played track across bag refills.
        if (paths.size() > 1 && bag.back() == lastPick) {
            std::swap(bag.front(), bag.back());
        }
    }
    const std::size_t index = bag.back();
    bag.pop_back();
    lastPick = index;
    return paths[index];
}

void AudioSystem::startBgm()
{
    if (!IsAudioDeviceReady()) {
        return;
    }
    stopBgm();

    // Draw the next track from each category's shuffle bag so every track plays
    // once before any repeats. Tracks are loaded here but not started; the state
    // machine in updateBgm plays whichever one the current state calls for, from
    // its beginning, at full volume (no fade-in).
    const std::string calm = nextBgmTrack(_calmPaths, _calmBag, _calmLast);
    if (!calm.empty()) {
        _bgmCalm = LoadMusicStream(calm.c_str());
        if (IsMusicValid(_bgmCalm)) {
            _bgmCalm.looping = true;
            _bgmCalmLoaded = true;
            SetMusicVolume(_bgmCalm, 0.0f);
        }
    }
    const std::string intense = nextBgmTrack(_intensePaths, _intenseBag, _intenseLast);
    if (!intense.empty()) {
        _bgmIntense = LoadMusicStream(intense.c_str());
        if (IsMusicValid(_bgmIntense)) {
            _bgmIntense.looping = true;
            _bgmIntenseLoaded = true;
            SetMusicVolume(_bgmIntense, 0.0f);
        }
    }

    _bgmActive = -1; // nothing playing yet
    _bgmCalmFade = 0.0f;
    _bgmIntenseFade = 0.0f;
    _bgmMaster = 1.0f;                   // no master fade-in; tracks hard-start themselves
    _bgmCalmDelay = kBgmCalmResumeDelay; // hold calm silent briefly; if the player
                                         // guns it during this window we start intense instead
}

void AudioSystem::updateBgm(float dt, const BgmState& state)
{
    if (!_bgmCalmLoaded && !_bgmIntenseLoaded) {
        return;
    }

    // Decide which track *should* be active this frame.
    //   during the post-reset delay: none, unless the player has already gone
    //     intense (then start intense right away);
    //   after the delay: intense if the run is intense, otherwise calm.
    if (_bgmCalmDelay > 0.0f && !state.intense) {
        _bgmCalmDelay = std::max(0.0f, _bgmCalmDelay - dt);
    }
    int desired;
    if (state.intense) {
        desired = 1; // intense
    } else if (_bgmCalmDelay > 0.0f) {
        desired = -1; // still holding: nothing yet
    } else {
        desired = 0; // calm
    }

    // On a state change, hard-start the newly active track from its beginning at
    // full level (no fade-in); the previous track fades out via its fade level.
    if (desired != _bgmActive) {
        if (desired == 0 && _bgmCalmLoaded) {
            SeekMusicStream(_bgmCalm, 0.0f);
            PlayMusicStream(_bgmCalm);
            _bgmCalmFade = 1.0f;
        } else if (desired == 1 && _bgmIntenseLoaded) {
            SeekMusicStream(_bgmIntense, 0.0f);
            PlayMusicStream(_bgmIntense);
            _bgmIntenseFade = 1.0f;
        }
        _bgmActive = desired;
    }

    // Fade levels: the active track holds at 1, the inactive one fades to 0.
    const float fadeStep = dt / kBgmCrossfadeTime;
    auto fadeToward = [&](float& fade, bool active) {
        const float target = active ? 1.0f : 0.0f;
        if (fade < target) {
            fade = std::min(target, fade + fadeStep);
        } else {
            fade = std::max(target, fade - fadeStep);
        }
    };
    fadeToward(_bgmCalmFade, _bgmActive == 0);
    fadeToward(_bgmIntenseFade, _bgmActive == 1);

    // Master envelope: duck on pause, fade out on game over, silent when muted.
    const float masterTarget = _bgmMuted ? 0.0f : (!state.active ? 0.0f : (state.audible ? 1.0f : kBgmPauseDuck));
    _bgmMaster += (masterTarget - _bgmMaster) * frameFactor(kBgmMasterSmoothing, dt);

    if (_bgmCalmLoaded) {
        SetMusicVolume(_bgmCalm, kBgmVolume * _bgmMaster * _bgmCalmFade);
        // Only advance a stream that is (or was recently) audible, to keep the
        // faded-out track from drifting; stopping it once silent frees the buffer.
        if (_bgmCalmFade > 0.0f) {
            UpdateMusicStream(_bgmCalm);
        } else if (IsMusicStreamPlaying(_bgmCalm) && _bgmActive != 0) {
            StopMusicStream(_bgmCalm);
        }
    }
    if (_bgmIntenseLoaded) {
        SetMusicVolume(_bgmIntense, kBgmVolume * _bgmMaster * _bgmIntenseFade);
        if (_bgmIntenseFade > 0.0f) {
            UpdateMusicStream(_bgmIntense);
        } else if (IsMusicStreamPlaying(_bgmIntense) && _bgmActive != 1) {
            StopMusicStream(_bgmIntense);
        }
    }
}

void AudioSystem::setBgmMuted(bool muted)
{
    _bgmMuted = muted;
}

bool AudioSystem::bgmMuted() const
{
    return _bgmMuted;
}

void AudioSystem::setSfxMuted(bool muted)
{
    _sfxMuted = muted;
}

bool AudioSystem::sfxMuted() const
{
    return _sfxMuted;
}

void AudioSystem::resetEngine()
{
    if (!_engineLoaded) {
        return;
    }
    _level = 0.0f;
    _volume = 0.0f;
    SetMusicVolume(_engine, 0.0f);
    SetMusicPitch(_engine, kIdlePitch);
    SeekMusicStream(_engine, 0.0f);
}

void AudioSystem::updateEngine(float dt, const EngineState& state)
{
    if (!_engineLoaded) {
        return;
    }

    // Drive level (0..1) drives pitch. Actual speed is the main driver so jumps,
    // flight and landings shift the engine note (environment feedback), with
    // throttle/brake adding punch on top of the player's input.
    float target = clampf(state.speed / kSpeedForMax, 0.0f, 1.0f) * kSpeedWeight;
    if (state.throttle) {
        target += kThrottleBump;
    }
    if (state.brake) {
        target += kBrakeBump;
    }
    target = clampf(target, 0.0f, 1.0f);
    if (!state.on) {
        target = 0.0f;
    }

    // Engine idles audibly whenever it is on (from game start); muted on
    // pause / game over.
    const float targetVolume = state.on ? (kIdleVolume + (kFullVolume - kIdleVolume) * target) : 0.0f;

    _level += (target - _level) * frameFactor(kLevelSmoothing, dt);
    _volume += (targetVolume - _volume) * frameFactor(kVolumeSmoothing, dt);

    SetMusicPitch(_engine, kIdlePitch + (kMaxPitch - kIdlePitch) * _level);
    SetMusicVolume(_engine, _sfxMuted ? 0.0f : _volume);
    UpdateMusicStream(_engine);
}

} // namespace ridge_dash

#else // !RIDGEDASH_ENABLE_AUDIO — no-op on DRM/FBDEV

namespace ridge_dash {

AudioSystem::~AudioSystem() = default;
void AudioSystem::load() {}
void AudioSystem::unload() {}
void AudioSystem::resetEngine() {}
void AudioSystem::updateEngine(float, const EngineState&) {}
void AudioSystem::play(Sfx) {}
void AudioSystem::startBgm() {}
void AudioSystem::updateBgm(float, const BgmState&) {}
void AudioSystem::setBgmMuted(bool) {}
bool AudioSystem::bgmMuted() const
{
    return false;
}
void AudioSystem::setSfxMuted(bool) {}
bool AudioSystem::sfxMuted() const
{
    return false;
}

} // namespace ridge_dash

#endif

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

#include <array>
#include <cmath>
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

// Framerate-independent lerp factor (matches the camera/engine smoothing elsewhere).
float frameFactor(float k, float dt)
{
    return 1.0f - std::pow(1.0f - k, dt / kPhysicsStep);
}

// Candidate on-disk locations for an audio file relative to assets/audio/.
std::array<std::string, 5> audioCandidates(const std::string& rel)
{
    const std::string appDir = GetApplicationDirectory();
    return {
        "assets/audio/" + rel,
        appDir + "assets/audio/" + rel,
        appDir + "../assets/audio/" + rel,
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
    const std::string appDir = GetApplicationDirectory();
    const std::array<std::string, 5> candidates = {
        std::string("assets/audio/engine_loop.wav"),
        appDir + "assets/audio/engine_loop.wav",
        appDir + "../assets/audio/engine_loop.wav",
        appDir + "../share/ridgedash/audio/engine_loop.wav",
        std::string("/usr/share/APPLaunch/share/ridgedash/audio/engine_loop.wav"),
    };
    for (const std::string& path : candidates) {
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
    const std::string appDir = GetApplicationDirectory();
    const std::array<std::string, 5> candidates = {
        std::string("assets/audio/sfx/") + fileName,
        appDir + "assets/audio/sfx/" + fileName,
        appDir + "../assets/audio/sfx/" + fileName,
        appDir + "../share/ridgedash/audio/sfx/" + fileName,
        std::string("/usr/share/APPLaunch/share/ridgedash/audio/sfx/") + fileName,
    };
    for (const std::string& path : candidates) {
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

void AudioSystem::startBgm()
{
    if (!IsAudioDeviceReady()) {
        return;
    }
    stopBgm();

    // Pick a fresh random track from each category for this run; both loop
    // continuously and are crossfaded by updateBgm.
    auto pick = [&](const std::vector<std::string>& paths) -> std::string {
        if (paths.empty()) {
            return {};
        }
        const std::size_t i =
            paths.size() == 1 ? 0 : std::uniform_int_distribution<std::size_t>(0, paths.size() - 1)(_rng);
        return paths[i];
    };

    const std::string calm = pick(_calmPaths);
    if (!calm.empty()) {
        _bgmCalm = LoadMusicStream(calm.c_str());
        if (IsMusicValid(_bgmCalm)) {
            _bgmCalm.looping = true;
            _bgmCalmLoaded = true;
            SetMusicVolume(_bgmCalm, 0.0f);
            PlayMusicStream(_bgmCalm);
        }
    }
    const std::string intense = pick(_intensePaths);
    if (!intense.empty()) {
        _bgmIntense = LoadMusicStream(intense.c_str());
        if (IsMusicValid(_bgmIntense)) {
            _bgmIntense.looping = true;
            _bgmIntenseLoaded = true;
            SetMusicVolume(_bgmIntense, 0.0f);
            PlayMusicStream(_bgmIntense);
        }
    }

    _bgmBlend = 0.0f;  // start on the calm track
    _bgmMaster = 0.0f; // fade in from silence
}

void AudioSystem::updateBgm(float dt, const BgmState& state)
{
    if (!_bgmCalmLoaded && !_bgmIntenseLoaded) {
        return;
    }

    // Crossfade blend (linear over kBgmCrossfadeTime) and master envelope
    // (duck on pause, fade out on game over).
    const float blendTarget = state.intense ? 1.0f : 0.0f;
    const float blendStep = dt / kBgmCrossfadeTime;
    if (_bgmBlend < blendTarget) {
        _bgmBlend = std::min(blendTarget, _bgmBlend + blendStep);
    } else {
        _bgmBlend = std::max(blendTarget, _bgmBlend - blendStep);
    }

    const float masterTarget = !state.active ? 0.0f : (state.audible ? 1.0f : kBgmPauseDuck);
    _bgmMaster += (masterTarget - _bgmMaster) * frameFactor(kBgmMasterSmoothing, dt);

    if (_bgmCalmLoaded) {
        SetMusicVolume(_bgmCalm, kBgmVolume * _bgmMaster * (1.0f - _bgmBlend));
        UpdateMusicStream(_bgmCalm);
    }
    if (_bgmIntenseLoaded) {
        SetMusicVolume(_bgmIntense, kBgmVolume * _bgmMaster * _bgmBlend);
        UpdateMusicStream(_bgmIntense);
    }
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
    SetMusicVolume(_engine, _volume);
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

} // namespace ridge_dash

#endif

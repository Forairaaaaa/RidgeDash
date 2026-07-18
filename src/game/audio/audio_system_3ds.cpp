#if defined(RIDGEDASH_3DS)

#include "game/audio/audio_system.hpp"

#include "game/game_config.hpp"

extern "C" {
#include <3ds/types.h>
#include <3ds/allocator/linear.h>
#include <3ds/ndsp/ndsp.h>
#include <3ds/ndsp/channel.h>
#include <3ds/result.h>
#include <3ds/services/dsp.h>
}

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <new>
#include <random>
#include <vector>

namespace ridge_dash {
namespace {

using game_config::clampf;
using game_config::kPhysicsStep;

constexpr int kEngineChannel = 0;
constexpr int kFirstSfxChannel = 1;
constexpr int kSfxVoiceCount = 8;

constexpr float kIdlePitch = 0.74f;
constexpr float kMaxPitch = 1.92f;
constexpr float kSpeedForMax = 24.0f;
constexpr float kSpeedWeight = 0.82f;
constexpr float kThrottleBump = 0.30f;
constexpr float kBrakeBump = 0.12f;
constexpr float kIdleVolume = 0.50f;
constexpr float kFullVolume = 1.20f;
constexpr float kLevelSmoothing = 0.16f;
constexpr float kVolumeSmoothing = 0.20f;
constexpr float kEngineOutputGain = 0.55f;
constexpr float kSfxOutputGain = 0.82f;

struct PcmSample {
    s16* data = nullptr;
    u32 sampleCount = 0;
    float sampleRate = 0.0f;
};

struct SfxVoice {
    ndspWaveBuf wave{};
};

uint16_t readU16(const uint8_t* data)
{
    return static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
}

uint32_t readU32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

int32_t decodePcm(const uint8_t* data, int bitsPerSample)
{
    switch (bitsPerSample) {
        case 8:
            return (static_cast<int32_t>(data[0]) - 128) << 8;
        case 16:
            return static_cast<int16_t>(readU16(data));
        case 24: {
            int32_t value = static_cast<int32_t>(data[0]) | (static_cast<int32_t>(data[1]) << 8) |
                            (static_cast<int32_t>(data[2]) << 16);
            if ((value & 0x00800000) != 0) {
                value |= static_cast<int32_t>(0xFF000000);
            }
            return value >> 8;
        }
        case 32:
            return static_cast<int32_t>(readU32(data)) >> 16;
        default:
            return 0;
    }
}

bool loadWav(const char* path, PcmSample& sample)
{
    FILE* file = std::fopen(path, "rb");
    if (!file) {
        return false;
    }

    std::array<uint8_t, 12> riff{};
    if (std::fread(riff.data(), 1, riff.size(), file) != riff.size() || std::memcmp(riff.data(), "RIFF", 4) != 0 ||
        std::memcmp(riff.data() + 8, "WAVE", 4) != 0) {
        std::fclose(file);
        return false;
    }

    uint16_t format = 0;
    uint16_t channels = 0;
    uint16_t bitsPerSample = 0;
    uint32_t sampleRate = 0;
    std::vector<uint8_t> pcm;

    std::array<uint8_t, 8> chunkHeader{};
    while (std::fread(chunkHeader.data(), 1, chunkHeader.size(), file) == chunkHeader.size()) {
        const uint32_t chunkSize = readU32(chunkHeader.data() + 4);
        if (std::memcmp(chunkHeader.data(), "fmt ", 4) == 0) {
            std::vector<uint8_t> chunk(chunkSize);
            if (chunkSize < 16 || std::fread(chunk.data(), 1, chunkSize, file) != chunkSize) {
                break;
            }
            format = readU16(chunk.data());
            channels = readU16(chunk.data() + 2);
            sampleRate = readU32(chunk.data() + 4);
            bitsPerSample = readU16(chunk.data() + 14);
        } else if (std::memcmp(chunkHeader.data(), "data", 4) == 0) {
            pcm.resize(chunkSize);
            if (std::fread(pcm.data(), 1, chunkSize, file) != chunkSize) {
                pcm.clear();
                break;
            }
        } else if (std::fseek(file, static_cast<long>(chunkSize), SEEK_CUR) != 0) {
            break;
        }
        if ((chunkSize & 1u) != 0) {
            std::fseek(file, 1, SEEK_CUR);
        }
    }
    std::fclose(file);

    const int bytesPerChannel = bitsPerSample / 8;
    const int bytesPerFrame = bytesPerChannel * channels;
    if (format != 1 || channels == 0 || channels > 2 || sampleRate == 0 ||
        (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) || pcm.empty() ||
        bytesPerFrame <= 0) {
        return false;
    }

    const size_t frameCount = pcm.size() / static_cast<size_t>(bytesPerFrame);
    if (frameCount == 0 || frameCount > UINT32_MAX) {
        return false;
    }

    auto* output = static_cast<s16*>(linearAlloc(frameCount * sizeof(s16)));
    if (!output) {
        return false;
    }

    for (size_t frame = 0; frame < frameCount; ++frame) {
        const uint8_t* input = pcm.data() + frame * bytesPerFrame;
        int32_t value = decodePcm(input, bitsPerSample);
        if (channels == 2) {
            value = (value + decodePcm(input + bytesPerChannel, bitsPerSample)) / 2;
        }
        output[frame] =
            static_cast<s16>(std::clamp(value, static_cast<int32_t>(INT16_MIN), static_cast<int32_t>(INT16_MAX)));
    }

    DSP_FlushDataCache(output, frameCount * sizeof(s16));
    sample.data = output;
    sample.sampleCount = static_cast<u32>(frameCount);
    sample.sampleRate = static_cast<float>(sampleRate);
    return true;
}

void freeSample(PcmSample& sample)
{
    if (sample.data) {
        linearFree(sample.data);
    }
    sample = {};
}

void setChannelVolume(int channel, float volume)
{
    std::array<float, 12> mix{};
    mix[0] = volume;
    mix[1] = volume;
    ndspChnSetMix(channel, mix.data());
}

void configurePcmChannel(int channel, float sampleRate)
{
    ndspChnSetInterp(channel, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(channel, sampleRate);
    ndspChnSetFormat(channel, NDSP_FORMAT_MONO_PCM16);
}

float frameFactor(float amount, float dt)
{
    return 1.0f - std::pow(1.0f - amount, dt / kPhysicsStep);
}

} // namespace

struct AudioSystem::ThreeDsAudioState {
    bool ndspReady = false;
    PcmSample engine{};
    ndspWaveBuf engineWave{};
    float engineLevel = 0.0f;
    float engineVolume = 0.0f;
    std::array<std::vector<PcmSample>, static_cast<size_t>(Sfx::Count)> sfx{};
    std::array<SfxVoice, kSfxVoiceCount> voices{};
    size_t nextVoice = 0;
    std::mt19937 rng{0xA17D};
};

AudioSystem::~AudioSystem()
{
    unload();
}

void AudioSystem::load()
{
    if (_threeDs) {
        return;
    }

    _threeDs = new (std::nothrow) ThreeDsAudioState();
    if (!_threeDs) {
        return;
    }
    if (R_FAILED(ndspInit())) {
        delete _threeDs;
        _threeDs = nullptr;
        return;
    }
    _threeDs->ndspReady = true;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspSetClippingMode(NDSP_CLIP_SOFT);

    if (loadWav("romfs:/audio/engine_loop.wav", _threeDs->engine)) {
        configurePcmChannel(kEngineChannel, _threeDs->engine.sampleRate * kIdlePitch);
        setChannelVolume(kEngineChannel, 0.0f);
        _threeDs->engineWave.data_pcm16 = _threeDs->engine.data;
        _threeDs->engineWave.nsamples = _threeDs->engine.sampleCount;
        _threeDs->engineWave.looping = true;
        ndspChnWaveBufAdd(kEngineChannel, &_threeDs->engineWave);
    }

    auto loadGroup = [&](Sfx id, std::initializer_list<const char*> files) {
        auto& group = _threeDs->sfx[static_cast<size_t>(id)];
        for (const char* file : files) {
            PcmSample sample{};
            char path[128]{};
            std::snprintf(path, sizeof(path), "romfs:/audio/sfx/%s", file);
            if (loadWav(path, sample)) {
                group.push_back(sample);
            }
        }
    };

    loadGroup(Sfx::Coin, {"coin_pickup_1.wav", "coin_pickup_2.wav"});
    loadGroup(Sfx::Fuel, {"fuel_can_pickup.wav"});
    loadGroup(Sfx::Cactus, {"cactus_pickup.wav"});
    loadGroup(Sfx::Snowman, {"snowman_pickup.wav"});
    loadGroup(Sfx::Rocket, {"rocket_pick.wav"});
    loadGroup(Sfx::RocketFinish, {"rocket_finish.wav"});
    loadGroup(Sfx::FleaJump, {"flea_jump.wav"});
    loadGroup(Sfx::CarLanding, {"car_landing_1.wav", "car_landing_2.wav", "car_landing_3.wav"});
    loadGroup(Sfx::CarHit, {"car_hit_1.wav", "car_hit_2.wav", "car_hit_3.wav"});
    loadGroup(Sfx::CarFlip, {"car_flip.wav"});
    loadGroup(Sfx::Reset, {"reset.wav"});
    loadGroup(Sfx::Shutdown, {"shutdown.wav"});
    loadGroup(Sfx::UiSelect, {"ui_select.wav"});
}

void AudioSystem::unload()
{
    if (!_threeDs) {
        return;
    }

    if (_threeDs->ndspReady) {
        for (int channel = kEngineChannel; channel < kFirstSfxChannel + kSfxVoiceCount; ++channel) {
            ndspChnWaveBufClear(channel);
            ndspChnReset(channel);
        }
    }
    freeSample(_threeDs->engine);
    for (auto& group : _threeDs->sfx) {
        for (PcmSample& sample : group) {
            freeSample(sample);
        }
        group.clear();
    }
    if (_threeDs->ndspReady) {
        ndspExit();
    }
    delete _threeDs;
    _threeDs = nullptr;
}

void AudioSystem::resetEngine()
{
    if (!_threeDs || !_threeDs->engine.data) {
        return;
    }
    _threeDs->engineLevel = 0.0f;
    _threeDs->engineVolume = 0.0f;
    ndspChnWaveBufClear(kEngineChannel);
    std::memset(&_threeDs->engineWave, 0, sizeof(_threeDs->engineWave));
    _threeDs->engineWave.data_pcm16 = _threeDs->engine.data;
    _threeDs->engineWave.nsamples = _threeDs->engine.sampleCount;
    _threeDs->engineWave.looping = true;
    configurePcmChannel(kEngineChannel, _threeDs->engine.sampleRate * kIdlePitch);
    setChannelVolume(kEngineChannel, 0.0f);
    ndspChnWaveBufAdd(kEngineChannel, &_threeDs->engineWave);
}

void AudioSystem::updateEngine(float dt, const EngineState& state)
{
    if (!_threeDs || !_threeDs->engine.data) {
        return;
    }

    float target = clampf(state.speed / kSpeedForMax, 0.0f, 1.0f) * kSpeedWeight;
    if (state.throttle) {
        target += kThrottleBump;
    }
    if (state.brake) {
        target += kBrakeBump;
    }
    target = state.on ? clampf(target, 0.0f, 1.0f) : 0.0f;
    const float targetVolume = state.on ? kIdleVolume + (kFullVolume - kIdleVolume) * target : 0.0f;

    _threeDs->engineLevel += (target - _threeDs->engineLevel) * frameFactor(kLevelSmoothing, dt);
    _threeDs->engineVolume += (targetVolume - _threeDs->engineVolume) * frameFactor(kVolumeSmoothing, dt);

    const float pitch = kIdlePitch + (kMaxPitch - kIdlePitch) * _threeDs->engineLevel;
    ndspChnSetRate(kEngineChannel, _threeDs->engine.sampleRate * pitch);
    const float volume = _sfxMuted ? 0.0f : clampf(_threeDs->engineVolume * kEngineOutputGain, 0.0f, 1.0f);
    setChannelVolume(kEngineChannel, volume);
}

void AudioSystem::play(Sfx id)
{
    if (!_threeDs || _sfxMuted) {
        return;
    }
    auto& group = _threeDs->sfx[static_cast<size_t>(id)];
    if (group.empty()) {
        return;
    }

    const size_t sampleIndex =
        group.size() == 1 ? 0 : std::uniform_int_distribution<size_t>(0, group.size() - 1)(_threeDs->rng);
    size_t voiceIndex = _threeDs->nextVoice;
    for (size_t i = 0; i < _threeDs->voices.size(); ++i) {
        const size_t candidate = (_threeDs->nextVoice + i) % _threeDs->voices.size();
        if (!ndspChnIsPlaying(kFirstSfxChannel + static_cast<int>(candidate))) {
            voiceIndex = candidate;
            break;
        }
    }
    _threeDs->nextVoice = (voiceIndex + 1) % _threeDs->voices.size();

    const int channel = kFirstSfxChannel + static_cast<int>(voiceIndex);
    const PcmSample& sample = group[sampleIndex];
    SfxVoice& voice = _threeDs->voices[voiceIndex];
    ndspChnWaveBufClear(channel);
    std::memset(&voice.wave, 0, sizeof(voice.wave));
    voice.wave.data_pcm16 = sample.data;
    voice.wave.nsamples = sample.sampleCount;
    configurePcmChannel(channel, sample.sampleRate);
    setChannelVolume(channel, kSfxOutputGain);
    ndspChnWaveBufAdd(channel, &voice.wave);
}

void AudioSystem::startBgm() {}
void AudioSystem::updateBgm(float, const BgmState&) {}

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

} // namespace ridge_dash

#endif // RIDGEDASH_3DS

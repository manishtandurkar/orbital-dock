#include "audio/AudioEngine.h"

#include "core/Assets.h"

#include <miniaudio.h>

#include <array>
#include <iostream>
#include <string>

namespace {

constexpr size_t kSoundCount = 8;

const char* soundFile(Sound sound)
{
    switch (sound) {
    case Sound::Thruster:
        return "audio/thruster_loop.wav";
    case Sound::DockingClunk:
        return "audio/docking_clunk.wav";
    case Sound::Impact:
        return "audio/impact.wav";
    case Sound::HullAlarm:
        return "audio/hull_alarm.wav";
    case Sound::AmbientHum:
        return "audio/ambient_hum.wav";
    case Sound::RepairComplete:
        return "audio/repair_complete.wav";
    case Sound::RepairWeld:
        return "audio/repair_weld.wav";
    case Sound::LockTone:
        return "audio/lock_tone.wav";
    }
    return "";
}

size_t soundIndex(Sound sound)
{
    return static_cast<size_t>(sound);
}

}

struct AudioEngine::Impl {
    ma_engine engine{};
    bool engineReady = false;
    std::array<ma_sound, kSoundCount> loops{};
    std::array<bool, kSoundCount> loopReady{};
    std::array<std::string, kSoundCount> paths;

    Impl()
    {
        if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
            std::cerr << "Audio disabled: no playback device available\n";
            return;
        }
        engineReady = true;
        for (size_t i = 0; i < kSoundCount; ++i) {
            paths[i] = assets::path(soundFile(static_cast<Sound>(i))).string();
            const ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION;
            if (ma_sound_init_from_file(&engine, paths[i].c_str(), flags, nullptr, nullptr, &loops[i]) == MA_SUCCESS) {
                loopReady[i] = true;
            } else {
                std::cerr << "Cannot load sound: " << paths[i] << '\n';
            }
        }
    }

    ~Impl()
    {
        if (!engineReady) {
            return;
        }
        for (size_t i = 0; i < kSoundCount; ++i) {
            if (loopReady[i]) {
                ma_sound_uninit(&loops[i]);
            }
        }
        ma_engine_uninit(&engine);
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
};

AudioEngine::AudioEngine()
    : impl_(std::make_unique<Impl>())
{
}

AudioEngine::~AudioEngine() = default;

bool AudioEngine::isAvailable() const
{
    return impl_->engineReady;
}

void AudioEngine::playOneShot(Sound sound, float volume)
{
    if (!impl_->engineReady) {
        return;
    }
    const size_t index = soundIndex(sound);
    if (!impl_->loopReady[index]) {
        return;
    }
    ma_sound& voice = impl_->loops[index];
    ma_sound_set_looping(&voice, MA_FALSE);
    ma_sound_set_volume(&voice, volume);
    ma_sound_seek_to_pcm_frame(&voice, 0);
    ma_sound_start(&voice);
}

void AudioEngine::setLoop(Sound sound, bool playing, float volume)
{
    if (!impl_->engineReady) {
        return;
    }
    const size_t index = soundIndex(sound);
    if (!impl_->loopReady[index]) {
        return;
    }
    ma_sound& voice = impl_->loops[index];
    ma_sound_set_volume(&voice, volume);
    const bool isPlaying = ma_sound_is_playing(&voice) == MA_TRUE;
    if (playing && !isPlaying) {
        ma_sound_set_looping(&voice, MA_TRUE);
        ma_sound_start(&voice);
    } else if (!playing && isPlaying) {
        ma_sound_stop(&voice);
    }
}

void AudioEngine::stopAllLoops()
{
    if (!impl_->engineReady) {
        return;
    }
    for (size_t i = 0; i < kSoundCount; ++i) {
        if (impl_->loopReady[i] && ma_sound_is_looping(&impl_->loops[i]) == MA_TRUE) {
            ma_sound_stop(&impl_->loops[i]);
        }
    }
}

void AudioEngine::setMasterVolume(float volume)
{
    if (impl_->engineReady) {
        ma_engine_set_volume(&impl_->engine, volume);
    }
}

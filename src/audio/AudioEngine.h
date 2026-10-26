#pragma once

#include <memory>
#include <vector>

enum class Sound {
    Thruster,
    DockingClunk,
    Impact,
    HullAlarm,
    AmbientHum,
    RepairComplete,
    RepairWeld,
    LockTone
};

class AudioEngine {
public:
    static constexpr int kSampleRate = 48000;
    static constexpr int kChannels = 2;

    explicit AudioEngine(bool offline = false);
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool isAvailable() const;
    void playOneShot(Sound sound, float volume = 1.0f);
    void setLoop(Sound sound, bool playing, float volume = 1.0f);
    void stopAllLoops();
    void setMasterVolume(float volume);
    void renderOffline(int frameCount, std::vector<float>& interleavedOut);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

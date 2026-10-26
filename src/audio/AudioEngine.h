#pragma once

#include <memory>

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
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool isAvailable() const;
    void playOneShot(Sound sound, float volume = 1.0f);
    void setLoop(Sound sound, bool playing, float volume = 1.0f);
    void stopAllLoops();
    void setMasterVolume(float volume);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

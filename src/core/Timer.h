#pragma once

class Timer {
public:
    Timer();

    float tick();
    double elapsedSeconds() const;
    float framesPerSecond() const;

private:
    static constexpr float kMaxFrameSeconds = 1.0f / 20.0f;

    double startTime_;
    double lastTime_;
    float smoothedFrameSeconds_ = 1.0f / 60.0f;
};

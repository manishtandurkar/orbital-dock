#pragma once

class Timer {
public:
    Timer();

    float tick();
    void useFixedStep(float stepSeconds);
    double elapsedSeconds() const;
    float framesPerSecond() const;

private:
    static constexpr float kMaxFrameSeconds = 1.0f / 20.0f;

    double startTime_;
    double lastTime_;
    float smoothedFrameSeconds_ = 1.0f / 60.0f;
    float fixedStepSeconds_ = 0.0f;
    double lastRealTime_;
};

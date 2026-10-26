#include "core/Timer.h"

#include <GLFW/glfw3.h>

#include <algorithm>

Timer::Timer()
    : startTime_(glfwGetTime())
    , lastTime_(startTime_)
    , lastRealTime_(startTime_)
{
}

void Timer::useFixedStep(float stepSeconds)
{
    fixedStepSeconds_ = stepSeconds;
}

float Timer::tick()
{
    const double now = glfwGetTime();
    const float frameSeconds = static_cast<float>(now - lastRealTime_);
    lastRealTime_ = now;
    smoothedFrameSeconds_ = smoothedFrameSeconds_ * 0.95f + frameSeconds * 0.05f;
    if (fixedStepSeconds_ > 0.0f) {
        lastTime_ += fixedStepSeconds_;
        return fixedStepSeconds_;
    }
    lastTime_ = now;
    return std::clamp(frameSeconds, 0.0f, kMaxFrameSeconds);
}

double Timer::elapsedSeconds() const
{
    return lastTime_ - startTime_;
}

float Timer::framesPerSecond() const
{
    if (smoothedFrameSeconds_ <= 0.0f) {
        return 0.0f;
    }
    return 1.0f / smoothedFrameSeconds_;
}

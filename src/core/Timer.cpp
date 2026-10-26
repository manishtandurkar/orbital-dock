#include "core/Timer.h"

#include <GLFW/glfw3.h>

#include <algorithm>

Timer::Timer()
    : startTime_(glfwGetTime())
    , lastTime_(startTime_)
{
}

float Timer::tick()
{
    const double now = glfwGetTime();
    const float frameSeconds = static_cast<float>(now - lastTime_);
    lastTime_ = now;
    smoothedFrameSeconds_ = smoothedFrameSeconds_ * 0.95f + frameSeconds * 0.05f;
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

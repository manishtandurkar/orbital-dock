#include "gameplay/ChaseCamera.h"

#include "render/Camera.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

namespace {

constexpr glm::vec3 kChaseOffset{0.0f, 3.4f, 14.0f};
constexpr glm::vec3 kCockpitOffset{0.0f, 0.95f, -2.1f};
constexpr glm::vec3 kChaseLookAhead{0.0f, 1.2f, -14.0f};
constexpr float kPositionFollowRate = 7.0f;
constexpr float kRotationFollowRate = 9.0f;
constexpr float kTraumaDecayPerSecond = 1.4f;
constexpr float kMaxShakeMeters = 0.9f;
constexpr float kMaxShakeRadians = 0.06f;

float exponentialBlend(float rate, float deltaSeconds)
{
    return 1.0f - std::exp(-rate * deltaSeconds);
}

float wobble(double time, double frequency, double phase)
{
    return static_cast<float>(std::sin(time * frequency + phase) * 0.6 + std::sin(time * frequency * 2.3 + phase * 1.7) * 0.4);
}

}

void ChaseCamera::snapTo(const glm::vec3& targetPosition, const glm::quat& targetOrientation)
{
    eye_ = desiredEye(targetPosition, targetOrientation);
    orientation_ = targetOrientation;
    trauma_ = 0.0f;
}

void ChaseCamera::update(Camera& camera, const glm::vec3& targetPosition, const glm::quat& targetOrientation,
    float deltaSeconds, double elapsedSeconds)
{
    trauma_ = std::max(trauma_ - kTraumaDecayPerSecond * deltaSeconds, 0.0f);

    if (view_ == View::Cockpit) {
        eye_ = desiredEye(targetPosition, targetOrientation);
        orientation_ = targetOrientation;
    } else {
        eye_ = glm::mix(eye_, desiredEye(targetPosition, targetOrientation), exponentialBlend(kPositionFollowRate, deltaSeconds));
        orientation_ = glm::slerp(orientation_, targetOrientation, exponentialBlend(kRotationFollowRate, deltaSeconds));
    }

    const float shake = trauma_ * trauma_;
    const glm::vec3 eye = eye_ + orientation_ * shakeOffset(elapsedSeconds) * shake * kMaxShakeMeters;
    const glm::quat jitter = glm::quat(glm::vec3(wobble(elapsedSeconds, 31.0, 4.0), wobble(elapsedSeconds, 27.0, 1.0),
                                           wobble(elapsedSeconds, 23.0, 2.0))
        * shake * kMaxShakeRadians);

    if (view_ == View::Cockpit) {
        camera.setPosition(eye);
        camera.setOrientation(orientation_ * jitter);
        return;
    }
    const glm::vec3 lookTarget = targetPosition + orientation_ * kChaseLookAhead;
    camera.lookAt(eye, lookTarget, orientation_ * glm::vec3(0.0f, 1.0f, 0.0f));
    camera.setOrientation(camera.orientation() * jitter);
}

void ChaseCamera::addShake(float trauma)
{
    trauma_ = std::min(trauma_ + trauma, 1.0f);
}

void ChaseCamera::toggleView()
{
    view_ = view_ == View::Chase ? View::Cockpit : View::Chase;
}

ChaseCamera::View ChaseCamera::view() const
{
    return view_;
}

glm::vec3 ChaseCamera::desiredEye(const glm::vec3& targetPosition, const glm::quat& targetOrientation) const
{
    const glm::vec3 offset = view_ == View::Cockpit ? kCockpitOffset : kChaseOffset;
    return targetPosition + targetOrientation * offset;
}

glm::vec3 ChaseCamera::shakeOffset(double elapsedSeconds) const
{
    return {wobble(elapsedSeconds, 41.0, 0.0), wobble(elapsedSeconds, 37.0, 3.0), wobble(elapsedSeconds, 29.0, 5.0)};
}

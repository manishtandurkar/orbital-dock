#include "gameplay/EvaController.h"

#include "core/Input.h"
#include "render/Camera.h"
#include "scene/SceneNode.h"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {

constexpr float kDeckTop = 6.6f;
constexpr float kEyeHeight = 1.65f;
constexpr glm::vec3 kStartFeetPosition{0.0f, kDeckTop, 9.0f};
constexpr float kDeckHalfWidth = 1.85f;
constexpr float kDeckNearZ = 10.0f;
constexpr float kDeckFarZ = -9.9f;
constexpr float kWalkAcceleration = 10.0f;
constexpr float kMaxWalkSpeed = 2.6f;
constexpr float kWalkDampingRate = 4.0f;
constexpr float kMouseRadiansPerPixel = 0.0022f;
constexpr float kMaxPitchRadians = 1.35f;
constexpr float kInteractDistance = 2.6f;
constexpr float kInteractConeDegrees = 35.0f;
constexpr float kBobFrequency = 1.6f;
constexpr float kBobAmplitude = 0.04f;
constexpr float kFloatAmplitude = 0.03f;
constexpr glm::vec3 kToolOffset{0.24f, -0.22f, -0.5f};

float axis(const Input& input, int positiveKey, int negativeKey)
{
    float value = 0.0f;
    if (input.isKeyDown(positiveKey)) {
        value += 1.0f;
    }
    if (input.isKeyDown(negativeKey)) {
        value -= 1.0f;
    }
    return value;
}

}

void EvaController::reset()
{
    feetPosition_ = kStartFeetPosition;
    velocity_ = glm::vec3(0.0f);
    yawRadians_ = 0.0f;
    pitchRadians_ = -0.1f;
    bobPhase_ = 0.0f;
    repairProgress_ = 0.0f;
    distanceToPanel_ = 0.0f;
    canInteract_ = false;
    repairing_ = false;
    repairJustCompleted_ = false;
}

void EvaController::update(const Input& input, float deltaSeconds, const glm::vec3& panelLocalPosition, bool panelRepaired)
{
    const glm::vec2 mouse = input.mouseDelta();
    yawRadians_ -= mouse.x * kMouseRadiansPerPixel;
    pitchRadians_ = std::clamp(pitchRadians_ - mouse.y * kMouseRadiansPerPixel, -kMaxPitchRadians, kMaxPitchRadians);
    updateMovement(input, deltaSeconds);
    updateRepair(input, deltaSeconds, panelLocalPosition, panelRepaired);
}

void EvaController::updateMovement(const Input& input, float deltaSeconds)
{
    const float forwardInput = axis(input, GLFW_KEY_W, GLFW_KEY_S);
    const float strafeInput = axis(input, GLFW_KEY_D, GLFW_KEY_A);
    const glm::vec3 forward(-std::sin(yawRadians_), 0.0f, -std::cos(yawRadians_));
    const glm::vec3 right(std::cos(yawRadians_), 0.0f, -std::sin(yawRadians_));

    glm::vec3 wish = forward * forwardInput + right * strafeInput;
    if (glm::length(wish) > 1.0f) {
        wish = glm::normalize(wish);
    }
    if (repairing_) {
        wish = glm::vec3(0.0f);
    }
    velocity_ += wish * kWalkAcceleration * deltaSeconds;
    velocity_ *= std::exp(-kWalkDampingRate * deltaSeconds);
    const float speed = glm::length(velocity_);
    if (speed > kMaxWalkSpeed) {
        velocity_ *= kMaxWalkSpeed / speed;
    }

    feetPosition_ += velocity_ * deltaSeconds;
    feetPosition_.x = std::clamp(feetPosition_.x, -kDeckHalfWidth, kDeckHalfWidth);
    feetPosition_.z = std::clamp(feetPosition_.z, kDeckFarZ, kDeckNearZ);
    feetPosition_.y = kDeckTop;

    bobPhase_ += glm::length(velocity_) * kBobFrequency * deltaSeconds * glm::two_pi<float>() * 0.5f;
    bobPhase_ += deltaSeconds * 0.8f;
}

void EvaController::updateRepair(const Input& input, float deltaSeconds, const glm::vec3& panelLocalPosition, bool panelRepaired)
{
    repairJustCompleted_ = false;
    const glm::vec3 eye = eyeLocalPosition();
    const glm::vec3 toPanel = panelLocalPosition - eye;
    distanceToPanel_ = glm::length(toPanel);
    const glm::vec3 lookDirection = viewRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
    const float facing = distanceToPanel_ > 1e-3f ? glm::dot(lookDirection, toPanel / distanceToPanel_) : 1.0f;
    canInteract_ = !panelRepaired && distanceToPanel_ <= kInteractDistance
        && facing >= std::cos(glm::radians(kInteractConeDegrees));

    repairing_ = canInteract_ && input.isKeyDown(GLFW_KEY_E);
    if (repairing_) {
        repairProgress_ = std::min(repairProgress_ + deltaSeconds / kRepairSeconds, 1.0f);
        if (repairProgress_ >= 1.0f) {
            repairJustCompleted_ = true;
            repairing_ = false;
        }
    }
}

glm::quat EvaController::viewRotation() const
{
    return glm::angleAxis(yawRadians_, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::angleAxis(pitchRadians_, glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 EvaController::eyeLocalPosition() const
{
    const float bob = std::sin(bobPhase_) * kBobAmplitude + std::sin(bobPhase_ * 0.37f) * kFloatAmplitude;
    return feetPosition_ + glm::vec3(0.0f, kEyeHeight + bob, 0.0f);
}

void EvaController::applyToCamera(Camera& camera, const SceneNode& deck) const
{
    camera.setPosition(deck.localToWorldPoint(eyeLocalPosition()));
    camera.setOrientation(deck.worldRotation() * viewRotation());
}

glm::mat4 EvaController::toolWorldMatrix(const SceneNode& deck, double elapsedSeconds) const
{
    const float sway = static_cast<float>(std::sin(bobPhase_ * 1.0)) * 0.012f;
    glm::vec3 offset = kToolOffset + glm::vec3(sway, std::abs(sway) * 0.5f, 0.0f);
    if (repairing_) {
        const float jitter = static_cast<float>(std::sin(elapsedSeconds * 61.0) * 0.006);
        offset += glm::vec3(jitter, -jitter, -0.06f);
    }
    const glm::mat4 eyeLocal = glm::translate(glm::mat4(1.0f), eyeLocalPosition()) * glm::mat4_cast(viewRotation());
    return deck.worldMatrix() * eyeLocal * glm::translate(glm::mat4(1.0f), offset);
}

bool EvaController::canInteract() const
{
    return canInteract_;
}

bool EvaController::isRepairing() const
{
    return repairing_;
}

bool EvaController::repairJustCompleted() const
{
    return repairJustCompleted_;
}

float EvaController::repairProgress() const
{
    return repairProgress_;
}

float EvaController::distanceToPanel() const
{
    return distanceToPanel_;
}

bool EvaController::isMoving() const
{
    return glm::length(velocity_) > 0.3f;
}

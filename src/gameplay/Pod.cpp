#include "gameplay/Pod.h"

#include "core/Input.h"
#include "scene/SceneNode.h"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

namespace {

constexpr float kForwardAcceleration = 7.0f;
constexpr float kReverseAcceleration = 6.0f;
constexpr float kVerticalAcceleration = 5.0f;
constexpr float kBrakeDeceleration = 12.0f;
constexpr float kMaxSpeed = 30.0f;
constexpr float kYawAcceleration = 1.6f;
constexpr float kRollAcceleration = 2.0f;
constexpr float kMaxAngularSpeed = 1.4f;
constexpr float kAngularDampingRate = 2.8f;
constexpr float kMouseRadiansPerPixel = 0.0020f;
constexpr float kThrusterResponseRate = 6.0f;
constexpr float kPrecisionScale = 0.25f;

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

float exponentialBlend(float rate, float deltaSeconds)
{
    return 1.0f - std::exp(-rate * deltaSeconds);
}

}

PodControls PodControls::fromInput(const Input& input)
{
    PodControls controls;
    controls.forwardThrust = axis(input, GLFW_KEY_W, GLFW_KEY_S);
    controls.verticalThrust = axis(input, GLFW_KEY_R, GLFW_KEY_F);
    controls.yawInput = axis(input, GLFW_KEY_A, GLFW_KEY_D);
    controls.rollInput = axis(input, GLFW_KEY_Q, GLFW_KEY_E);
    controls.mouseLook = input.mouseDelta();
    controls.brake = input.isKeyDown(GLFW_KEY_SPACE);
    controls.precision = input.isKeyDown(GLFW_KEY_LEFT_SHIFT) || input.isKeyDown(GLFW_KEY_RIGHT_SHIFT);
    return controls;
}

bool PodControls::anyThrust() const
{
    return forwardThrust != 0.0f || verticalThrust != 0.0f || yawInput != 0.0f || rollInput != 0.0f || brake;
}

Pod::Pod(SceneNode& node)
    : node_(node)
{
}

void Pod::reset(const glm::vec3& position, const glm::quat& orientation)
{
    position_ = position;
    orientation_ = glm::normalize(orientation);
    velocity_ = glm::vec3(0.0f);
    angularVelocityBody_ = glm::vec3(0.0f);
    hull_ = kMaxHull;
    thrusterLevel_ = 0.0f;
    docked_ = false;
    node_.setScale(glm::vec3(1.0f));
    syncNode();
}

void Pod::placeAt(const glm::vec3& position, const glm::quat& orientation)
{
    position_ = position;
    orientation_ = glm::normalize(orientation);
    velocity_ = glm::vec3(0.0f);
    angularVelocityBody_ = glm::vec3(0.0f);
    syncNode();
}

void Pod::update(const PodControls& controls, float deltaSeconds, const glm::vec3& matchAngularVelocityWorld)
{
    if (docked_) {
        thrusterLevel_ = glm::mix(thrusterLevel_, 0.0f, exponentialBlend(kThrusterResponseRate, deltaSeconds));
        return;
    }
    integrateRotation(controls, deltaSeconds, matchAngularVelocityWorld);
    integrateTranslation(controls, deltaSeconds);

    const float targetLevel = controls.anyThrust() ? 1.0f : 0.0f;
    thrusterLevel_ = glm::mix(thrusterLevel_, targetLevel, exponentialBlend(kThrusterResponseRate, deltaSeconds));
    syncNode();
}

void Pod::integrateRotation(const PodControls& controls, float deltaSeconds, const glm::vec3& matchAngularVelocityWorld)
{
    const glm::vec3 targetBody = glm::conjugate(orientation_) * matchAngularVelocityWorld;
    const float scale = controls.precision ? kPrecisionScale : 1.0f;
    const float damping = exponentialBlend(kAngularDampingRate, deltaSeconds);

    if (controls.yawInput != 0.0f) {
        angularVelocityBody_.y += controls.yawInput * kYawAcceleration * scale * deltaSeconds;
    } else {
        angularVelocityBody_.y = glm::mix(angularVelocityBody_.y, targetBody.y, damping);
    }
    if (controls.rollInput != 0.0f) {
        angularVelocityBody_.z += controls.rollInput * kRollAcceleration * scale * deltaSeconds;
    } else {
        angularVelocityBody_.z = glm::mix(angularVelocityBody_.z, targetBody.z, damping);
    }
    angularVelocityBody_.x = glm::mix(angularVelocityBody_.x, targetBody.x, damping);
    if (controls.brake) {
        angularVelocityBody_ = glm::mix(angularVelocityBody_, targetBody, damping);
    }
    angularVelocityBody_ = glm::clamp(angularVelocityBody_, glm::vec3(-kMaxAngularSpeed), glm::vec3(kMaxAngularSpeed));

    const glm::vec3 rotationThisFrame = angularVelocityBody_ * deltaSeconds
        + glm::vec3(-controls.mouseLook.y, -controls.mouseLook.x, 0.0f) * kMouseRadiansPerPixel * (controls.precision ? 0.5f : 1.0f);
    const float angle = glm::length(rotationThisFrame);
    if (angle > 1e-7f) {
        orientation_ = glm::normalize(orientation_ * glm::angleAxis(angle, rotationThisFrame / angle));
    }
}

void Pod::integrateTranslation(const PodControls& controls, float deltaSeconds)
{
    glm::vec3 acceleration(0.0f);
    if (controls.forwardThrust > 0.0f) {
        acceleration += forward() * kForwardAcceleration * controls.forwardThrust;
    } else if (controls.forwardThrust < 0.0f) {
        acceleration += forward() * kReverseAcceleration * controls.forwardThrust;
    }
    acceleration += up() * kVerticalAcceleration * controls.verticalThrust;
    if (controls.precision) {
        acceleration *= kPrecisionScale;
    }
    velocity_ += acceleration * deltaSeconds;

    if (controls.brake) {
        const float speedNow = glm::length(velocity_);
        if (speedNow > 0.0f) {
            const float reduced = std::max(speedNow - kBrakeDeceleration * deltaSeconds, 0.0f);
            velocity_ *= reduced / speedNow;
        }
    }

    const float speedNow = glm::length(velocity_);
    if (speedNow > kMaxSpeed) {
        velocity_ *= kMaxSpeed / speedNow;
    }
    position_ += velocity_ * deltaSeconds;
}

void Pod::syncNode()
{
    if (docked_) {
        return;
    }
    node_.setPosition(position_);
    node_.setRotation(orientation_);
}

void Pod::applyDamage(float amount)
{
    hull_ = std::max(hull_ - amount, 0.0f);
}

void Pod::applyImpulse(const glm::vec3& deltaVelocity)
{
    velocity_ += deltaVelocity;
}

void Pod::pushOut(const glm::vec3& offset)
{
    position_ += offset;
    syncNode();
}

void Pod::setDocked(bool docked)
{
    docked_ = docked;
    if (docked) {
        velocity_ = glm::vec3(0.0f);
        angularVelocityBody_ = glm::vec3(0.0f);
    }
}

bool Pod::isDocked() const
{
    return docked_;
}

bool Pod::isDestroyed() const
{
    return hull_ <= 0.0f;
}

float Pod::hull() const
{
    return hull_;
}

float Pod::hullFraction() const
{
    return hull_ / kMaxHull;
}

float Pod::thrusterLevel() const
{
    return thrusterLevel_;
}

float Pod::speed() const
{
    return glm::length(velocity_);
}

const glm::vec3& Pod::position() const
{
    return position_;
}

const glm::vec3& Pod::velocity() const
{
    return velocity_;
}

const glm::quat& Pod::orientation() const
{
    return orientation_;
}

glm::vec3 Pod::forward() const
{
    return orientation_ * glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 Pod::up() const
{
    return orientation_ * glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 Pod::right() const
{
    return orientation_ * glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 Pod::worldPosition() const
{
    return node_.worldPosition();
}

glm::quat Pod::worldOrientation() const
{
    return node_.worldRotation();
}

glm::vec3 Pod::angularVelocityWorld() const
{
    return orientation_ * angularVelocityBody_;
}

SceneNode& Pod::node()
{
    return node_;
}

const SceneNode& Pod::node() const
{
    return node_;
}

#include "gameplay/Debris.h"

#include "scene/SceneNode.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>


Debris::Debris(SceneNode& node, float modelRadius)
    : node_(node)
    , modelRadius_(modelRadius)
{
    node_.setVisible(false);
}

void Debris::spawn(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& spinAxis, float spinSpeed, float scale)
{
    position_ = position;
    velocity_ = velocity;
    spinAxis_ = glm::normalize(spinAxis);
    spinSpeed_ = spinSpeed;
    scale_ = scale;
    tumbleSecondsLeft_ = 0.0f;
    state_ = DebrisState::Drifting;
    node_.setPosition(position_);
    node_.setScale(glm::vec3(scale_));
    node_.setVisible(true);
}

void Debris::update(float deltaSeconds)
{
    if (state_ == DebrisState::Despawned) {
        return;
    }
    position_ += velocity_ * deltaSeconds;
    if (state_ == DebrisState::Tumbling) {
        tumbleSecondsLeft_ -= deltaSeconds;
        if (tumbleSecondsLeft_ <= 0.0f) {
            state_ = DebrisState::Drifting;
        }
    }
    node_.setPosition(position_);
    node_.setRotation(glm::angleAxis(spinSpeed_ * deltaSeconds, spinAxis_) * node_.rotation());
}

void Debris::startTumbling(const glm::vec3& spinAxis, float spinSpeed)
{
    if (glm::length(spinAxis) > 1e-4f) {
        spinAxis_ = glm::normalize(spinAxis);
    }
    spinSpeed_ = spinSpeed;
    tumbleSecondsLeft_ = kTumbleSeconds;
    state_ = DebrisState::Tumbling;
}

void Debris::despawn()
{
    state_ = DebrisState::Despawned;
    node_.setVisible(false);
}

void Debris::setVelocity(const glm::vec3& velocity)
{
    velocity_ = velocity;
}

void Debris::pushOut(const glm::vec3& offset)
{
    position_ += offset;
    node_.setPosition(position_);
}

DebrisState Debris::state() const
{
    return state_;
}

bool Debris::isActive() const
{
    return state_ != DebrisState::Despawned;
}

const glm::vec3& Debris::position() const
{
    return position_;
}

const glm::vec3& Debris::velocity() const
{
    return velocity_;
}

float Debris::radius() const
{
    return modelRadius_ * scale_ * 0.85f;
}

float Debris::mass() const
{
    return scale_ * scale_ * scale_;
}

SceneNode& Debris::node()
{
    return node_;
}

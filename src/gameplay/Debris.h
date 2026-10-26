#pragma once

#include <glm/vec3.hpp>

class SceneNode;

enum class DebrisState {
    Drifting,
    Tumbling,
    Despawned
};

class Debris {
public:
    Debris(SceneNode& node, float modelRadius);

    void spawn(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& spinAxis, float spinSpeed, float scale);
    void update(float deltaSeconds);
    void startTumbling(const glm::vec3& spinAxis, float spinSpeed);
    void despawn();

    void setVelocity(const glm::vec3& velocity);
    void pushOut(const glm::vec3& offset);

    DebrisState state() const;
    bool isActive() const;
    const glm::vec3& position() const;
    const glm::vec3& velocity() const;
    float radius() const;
    float mass() const;
    SceneNode& node();

private:
    static constexpr float kTumbleSeconds = 5.0f;

    SceneNode& node_;
    float modelRadius_;
    float scale_ = 1.0f;
    glm::vec3 position_{0.0f};
    glm::vec3 velocity_{0.0f};
    glm::vec3 spinAxis_{0.0f, 1.0f, 0.0f};
    float spinSpeed_ = 0.0f;
    float tumbleSecondsLeft_ = 0.0f;
    DebrisState state_ = DebrisState::Despawned;
};

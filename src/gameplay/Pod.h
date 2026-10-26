#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class Input;
class SceneNode;

struct PodControls {
    float forwardThrust = 0.0f;
    float verticalThrust = 0.0f;
    float yawInput = 0.0f;
    float rollInput = 0.0f;
    glm::vec2 mouseLook{0.0f};
    bool brake = false;

    static PodControls fromInput(const Input& input);
    bool anyThrust() const;
};

class Pod {
public:
    static constexpr float kCollisionRadius = 2.0f;
    static constexpr float kMaxHull = 100.0f;

    explicit Pod(SceneNode& node);

    void reset(const glm::vec3& position, const glm::quat& orientation);
    void placeAt(const glm::vec3& position, const glm::quat& orientation);
    void update(const PodControls& controls, float deltaSeconds, const glm::vec3& matchAngularVelocityWorld);
    void syncNode();

    void applyDamage(float amount);
    void applyImpulse(const glm::vec3& deltaVelocity);
    void pushOut(const glm::vec3& offset);
    void setDocked(bool docked);

    bool isDocked() const;
    bool isDestroyed() const;
    float hull() const;
    float hullFraction() const;
    float thrusterLevel() const;
    float speed() const;

    const glm::vec3& position() const;
    const glm::vec3& velocity() const;
    const glm::quat& orientation() const;
    glm::vec3 forward() const;
    glm::vec3 up() const;
    glm::vec3 right() const;
    glm::vec3 worldPosition() const;
    glm::quat worldOrientation() const;
    glm::vec3 angularVelocityWorld() const;

    SceneNode& node();
    const SceneNode& node() const;

private:
    void integrateRotation(const PodControls& controls, float deltaSeconds, const glm::vec3& matchAngularVelocityWorld);
    void integrateTranslation(const PodControls& controls, float deltaSeconds);

    SceneNode& node_;
    glm::vec3 position_{0.0f};
    glm::vec3 velocity_{0.0f};
    glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 angularVelocityBody_{0.0f};
    float hull_ = kMaxHull;
    float thrusterLevel_ = 0.0f;
    bool docked_ = false;
};

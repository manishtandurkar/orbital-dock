#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera;
class Input;
class SceneNode;

class EvaController {
public:
    static constexpr float kRepairSeconds = 3.0f;

    void reset();
    void update(const Input& input, float deltaSeconds, const glm::vec3& panelLocalPosition, bool panelRepaired);
    void applyToCamera(Camera& camera, const SceneNode& deck) const;
    glm::mat4 toolWorldMatrix(const SceneNode& deck, double elapsedSeconds) const;

    bool canInteract() const;
    bool isRepairing() const;
    bool repairJustCompleted() const;
    float repairProgress() const;
    float distanceToPanel() const;
    bool isMoving() const;

private:
    glm::quat viewRotation() const;
    glm::vec3 eyeLocalPosition() const;
    void updateMovement(const Input& input, float deltaSeconds);
    void updateRepair(const Input& input, float deltaSeconds, const glm::vec3& panelLocalPosition, bool panelRepaired);

    glm::vec3 feetPosition_{0.0f};
    glm::vec3 velocity_{0.0f};
    float yawRadians_ = 0.0f;
    float pitchRadians_ = 0.0f;
    float bobPhase_ = 0.0f;
    float repairProgress_ = 0.0f;
    float distanceToPanel_ = 0.0f;
    bool canInteract_ = false;
    bool repairing_ = false;
    bool repairJustCompleted_ = false;
};

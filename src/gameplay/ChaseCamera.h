#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

class Camera;

class ChaseCamera {
public:
    enum class View {
        Chase,
        Cockpit
    };

    void snapTo(const glm::vec3& targetPosition, const glm::quat& targetOrientation);
    void update(Camera& camera, const glm::vec3& targetPosition, const glm::quat& targetOrientation, float deltaSeconds,
        double elapsedSeconds);
    void addShake(float trauma);
    void toggleView();
    View view() const;

private:
    glm::vec3 desiredEye(const glm::vec3& targetPosition, const glm::quat& targetOrientation) const;
    glm::vec3 shakeOffset(double elapsedSeconds) const;

    glm::vec3 eye_{0.0f};
    glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
    float trauma_ = 0.0f;
    View view_ = View::Chase;
};

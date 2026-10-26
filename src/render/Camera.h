#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera {
public:
    Camera();

    void setPosition(const glm::vec3& position);
    void setOrientation(const glm::quat& orientation);
    void lookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up);
    void setPerspective(float verticalFovDegrees, float nearPlane, float farPlane);
    void setAspectRatio(float aspectRatio);

    const glm::vec3& position() const;
    const glm::quat& orientation() const;
    glm::vec3 forward() const;
    glm::vec3 up() const;
    glm::vec3 right() const;
    float verticalFovDegrees() const;

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;
    glm::mat4 viewProjectionMatrix() const;

private:
    glm::vec3 position_{0.0f};
    glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
    float verticalFovDegrees_ = 60.0f;
    float nearPlane_ = 0.1f;
    float farPlane_ = 4000.0f;
    float aspectRatio_ = 16.0f / 9.0f;
};

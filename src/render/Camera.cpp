#include "render/Camera.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

Camera::Camera() = default;

void Camera::setPosition(const glm::vec3& position)
{
    position_ = position;
}

void Camera::setOrientation(const glm::quat& orientation)
{
    orientation_ = glm::normalize(orientation);
}

void Camera::lookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up)
{
    position_ = eye;
    const glm::vec3 direction = target - eye;
    if (glm::dot(direction, direction) < 1e-8f) {
        return;
    }
    orientation_ = glm::quatLookAt(glm::normalize(direction), glm::normalize(up));
}

void Camera::setPerspective(float verticalFovDegrees, float nearPlane, float farPlane)
{
    verticalFovDegrees_ = verticalFovDegrees;
    nearPlane_ = nearPlane;
    farPlane_ = farPlane;
}

void Camera::setAspectRatio(float aspectRatio)
{
    aspectRatio_ = aspectRatio;
}

const glm::vec3& Camera::position() const
{
    return position_;
}

const glm::quat& Camera::orientation() const
{
    return orientation_;
}

glm::vec3 Camera::forward() const
{
    return orientation_ * glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 Camera::up() const
{
    return orientation_ * glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 Camera::right() const
{
    return orientation_ * glm::vec3(1.0f, 0.0f, 0.0f);
}

float Camera::verticalFovDegrees() const
{
    return verticalFovDegrees_;
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::mat4_cast(glm::conjugate(orientation_)) * glm::translate(glm::mat4(1.0f), -position_);
}

glm::mat4 Camera::projectionMatrix() const
{
    return glm::perspective(glm::radians(verticalFovDegrees_), aspectRatio_, nearPlane_, farPlane_);
}

glm::mat4 Camera::viewProjectionMatrix() const
{
    return projectionMatrix() * viewMatrix();
}

#include "physics/DockingCheck.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace {

float angleBetweenDegrees(const glm::vec3& a, const glm::vec3& b)
{
    const float cosine = std::clamp(glm::dot(glm::normalize(a), glm::normalize(b)), -1.0f, 1.0f);
    return glm::degrees(std::acos(cosine));
}

glm::vec3 projectOntoPlane(const glm::vec3& vector, const glm::vec3& planeNormal)
{
    return vector - planeNormal * glm::dot(vector, planeNormal);
}

}

DockingCheck::DockingCheck(DockingTolerance tolerance)
    : tolerance_(tolerance)
{
}

DockingStatus DockingCheck::evaluate(const DockingInputs& inputs, float deltaSeconds)
{
    DockingStatus status;
    const glm::vec3 axis = glm::normalize(inputs.portAxis);
    const glm::vec3 portUp = glm::normalize(projectOntoPlane(inputs.portUp, axis));
    const glm::vec3 portRight = glm::cross(portUp, axis);

    const glm::vec3 offset = inputs.podPosition - inputs.targetPosition;
    status.distance = glm::length(offset);
    status.axialDistance = glm::dot(offset, axis);
    status.lateralOffset = {glm::dot(offset, portRight), glm::dot(offset, portUp)};

    const glm::vec3 relativeVelocity = inputs.podVelocity - inputs.targetVelocity;
    status.relativeSpeed = glm::length(relativeVelocity);
    status.closingSpeed = -glm::dot(relativeVelocity, axis);

    status.alignmentDegrees = angleBetweenDegrees(inputs.podForward, -axis);
    const glm::vec3 podUpInPlane = projectOntoPlane(inputs.podUp, axis);
    status.rollDegrees = glm::length(podUpInPlane) > 1e-4f ? angleBetweenDegrees(podUpInPlane, portUp) : 180.0f;

    status.distanceOk = status.distance <= tolerance_.maxDistance;
    status.speedOk = status.relativeSpeed <= tolerance_.maxRelativeSpeed;
    status.alignmentOk = status.alignmentDegrees <= tolerance_.maxAlignmentDegrees;
    status.rollOk = status.rollDegrees <= tolerance_.maxRollDegrees;

    if (status.allWithinTolerance()) {
        holdTimer_ += deltaSeconds;
    } else {
        holdTimer_ = 0.0f;
    }
    status.holdProgress = std::clamp(holdTimer_ / tolerance_.holdSeconds, 0.0f, 1.0f);
    status.locked = holdTimer_ >= tolerance_.holdSeconds;
    return status;
}

void DockingCheck::reset()
{
    holdTimer_ = 0.0f;
}

const DockingTolerance& DockingCheck::tolerance() const
{
    return tolerance_;
}

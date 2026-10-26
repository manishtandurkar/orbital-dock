#include "physics/Collision.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <cmath>

namespace collision {

namespace {

constexpr float kEpsilon = 1e-6f;

glm::vec3 axisOfLeastPenetration(const glm::vec3& point, const Aabb& box, float& depth)
{
    const glm::vec3 toMin = point - box.min;
    const glm::vec3 toMax = box.max - point;
    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    depth = INFINITY;
    for (int axis = 0; axis < 3; ++axis) {
        if (toMin[axis] < depth) {
            depth = toMin[axis];
            normal = glm::vec3(0.0f);
            normal[axis] = -1.0f;
        }
        if (toMax[axis] < depth) {
            depth = toMax[axis];
            normal = glm::vec3(0.0f);
            normal[axis] = 1.0f;
        }
    }
    return normal;
}

}

Contact sphereVsSphere(const Sphere& a, const Sphere& b)
{
    Contact contact;
    const glm::vec3 offset = b.center - a.center;
    const float distanceSquared = glm::dot(offset, offset);
    const float radii = a.radius + b.radius;
    if (distanceSquared >= radii * radii) {
        return contact;
    }
    const float distance = std::sqrt(distanceSquared);
    contact.hit = true;
    contact.normal = distance > kEpsilon ? offset / distance : glm::vec3(0.0f, 1.0f, 0.0f);
    contact.penetration = radii - distance;
    return contact;
}

glm::vec3 closestPointOnAabb(const glm::vec3& point, const Aabb& box)
{
    return glm::clamp(point, box.min, box.max);
}

Contact sphereVsAabb(const Sphere& sphere, const Aabb& box)
{
    Contact contact;
    const glm::vec3 closest = closestPointOnAabb(sphere.center, box);
    const glm::vec3 offset = sphere.center - closest;
    const float distanceSquared = glm::dot(offset, offset);
    if (distanceSquared >= sphere.radius * sphere.radius) {
        return contact;
    }
    contact.hit = true;
    if (distanceSquared > kEpsilon) {
        const float distance = std::sqrt(distanceSquared);
        contact.normal = offset / distance;
        contact.penetration = sphere.radius - distance;
    } else {
        float depth = 0.0f;
        contact.normal = axisOfLeastPenetration(sphere.center, box, depth);
        contact.penetration = sphere.radius + depth;
    }
    return contact;
}

float resolveImpulse(glm::vec3& velocityA, float inverseMassA, glm::vec3& velocityB, float inverseMassB,
    const glm::vec3& normal, float restitution)
{
    const float closingSpeed = glm::dot(velocityB - velocityA, normal);
    if (closingSpeed >= 0.0f) {
        return 0.0f;
    }
    const float inverseMassSum = inverseMassA + inverseMassB;
    if (inverseMassSum <= kEpsilon) {
        return 0.0f;
    }
    const float impulse = -(1.0f + restitution) * closingSpeed / inverseMassSum;
    velocityA -= normal * impulse * inverseMassA;
    velocityB += normal * impulse * inverseMassB;
    return -closingSpeed;
}

}

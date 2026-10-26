#pragma once

#include "physics/Aabb.h"
#include "physics/Sphere.h"

#include <glm/vec3.hpp>

struct Contact {
    bool hit = false;
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float penetration = 0.0f;
};

namespace collision {

Contact sphereVsSphere(const Sphere& a, const Sphere& b);

Contact sphereVsAabb(const Sphere& sphere, const Aabb& box);

glm::vec3 closestPointOnAabb(const glm::vec3& point, const Aabb& box);

float resolveImpulse(glm::vec3& velocityA, float inverseMassA, glm::vec3& velocityB, float inverseMassB,
    const glm::vec3& normal, float restitution);

}

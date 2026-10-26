#pragma once

#include <glm/vec3.hpp>

struct Aabb {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};

    glm::vec3 center() const
    {
        return (min + max) * 0.5f;
    }

    glm::vec3 extents() const
    {
        return (max - min) * 0.5f;
    }
};

#pragma once

#include <glm/vec3.hpp>

#include <vector>

struct DirectionalLight {
    glm::vec3 direction{-0.4f, -0.3f, -1.0f};
    glm::vec3 color{1.0f, 0.96f, 0.88f};
    float intensity = 2.2f;
};

struct PointLight {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 20.0f;
};

struct Lighting {
    static constexpr int kMaxPointLights = 8;

    DirectionalLight sun;
    std::vector<PointLight> pointLights;
    glm::vec3 ambientColor{0.06f, 0.065f, 0.08f};
    float exposure = 1.0f;
};

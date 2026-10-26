#pragma once

#include <glm/vec3.hpp>

#include <vector>

struct DirectionalLight {
    glm::vec3 direction{0.6f, -0.35f, -0.4f};
    glm::vec3 color{1.0f, 0.96f, 0.88f};
    float intensity = 1.9f;
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
    glm::vec3 ambientColor{0.018f, 0.02f, 0.026f};
    glm::vec3 fillDirection{0.05f, 0.75f, 0.65f};
    glm::vec3 fillColor{0.08f, 0.12f, 0.2f};
};

#pragma once

#include <glm/vec3.hpp>

class Texture;

struct Material {
    glm::vec3 diffuseColor{0.8f};
    glm::vec3 specularColor{0.5f};
    glm::vec3 emissiveColor{0.0f};
    float shininess = 32.0f;
    const Texture* diffuseMap = nullptr;
};

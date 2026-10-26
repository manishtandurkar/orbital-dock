#pragma once

#include "render/Material.h"
#include "render/Mesh.h"
#include "render/Texture.h"

#include <glm/vec3.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Model {
public:
    struct Part {
        Mesh mesh;
        Material material;
    };

    explicit Model(std::string_view assetPath);

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    const std::vector<Part>& parts() const;
    const glm::vec3& boundsMin() const;
    const glm::vec3& boundsMax() const;
    float boundingRadius() const;

private:
    const Texture* loadTexture(const std::string& path);

    std::vector<Part> parts_;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures_;
    glm::vec3 boundsMin_{0.0f};
    glm::vec3 boundsMax_{0.0f};
    float boundingRadius_ = 0.0f;
};

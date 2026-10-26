#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class ShadowMap {
public:
    explicit ShadowMap(int resolution);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void focus(const glm::vec3& center, float radius, const glm::vec3& lightDirection);
    void bindForWriting() const;
    void bindTexture(int unit) const;

    const glm::mat4& lightSpaceMatrix() const;
    int resolution() const;

private:
    int resolution_;
    unsigned int framebuffer_ = 0;
    unsigned int depthTexture_ = 0;
    glm::mat4 lightSpace_{1.0f};
};

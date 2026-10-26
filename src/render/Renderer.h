#pragma once

#include "render/Cubemap.h"
#include "render/DebugLines.h"
#include "render/Light.h"
#include "render/Mesh.h"
#include "render/Shader.h"
#include "render/ShadowMap.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class Camera;
class Model;
class ParticleSystem;

struct DrawStyle {
    glm::vec3 tint{1.0f};
    float emissiveStrength = 1.0f;
    bool unlit = false;
    float rimStrength = 0.0f;
    glm::vec3 rimColor{0.4f, 0.65f, 1.0f};
};

class Renderer {
public:
    Renderer();

    void beginShadowPass(const glm::vec3& center, float radius, const glm::vec3& lightDirection);
    void drawModelShadow(const Model& model, const glm::mat4& world);
    void endShadowPass();
    void clear(const glm::ivec2& framebufferSize);
    void beginView(const Camera& camera, const glm::ivec2& viewportOrigin, const glm::ivec2& viewportSize, const Lighting& lighting);
    void drawModel(const Model& model, const glm::mat4& world, const DrawStyle& style = {});
    void drawMesh(const Mesh& mesh, const glm::mat4& world, const glm::vec3& color, const DrawStyle& style = {});
    void drawParticles(ParticleSystem& particles);
    void endView();
    void clearDebugLines();

    DebugLines& debugLines();

private:
    void applyLighting(const Lighting& lighting);
    void setTransform(const glm::mat4& world);
    void drawSkybox();
    void drawDebugLines();

    Shader litShader_;
    Shader skyboxShader_;
    Shader lineShader_;
    Shader particleShader_;
    Shader shadowShader_;
    ShadowMap shadowMap_;
    bool shadowsReady_ = false;
    Cubemap sky_;
    Mesh skyCube_;
    DebugLines debugLines_;
    glm::mat4 view_{1.0f};
    glm::mat4 projection_{1.0f};
    float viewportHeight_ = 1.0f;
    DirectionalLight sun_;
};

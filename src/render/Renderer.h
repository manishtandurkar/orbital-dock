#pragma once

#include "render/Cubemap.h"
#include "render/DebugLines.h"
#include "render/Light.h"
#include "render/Mesh.h"
#include "render/Shader.h"

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
};

class Renderer {
public:
    Renderer();

    void beginFrame(const Camera& camera, const glm::ivec2& framebufferSize, const Lighting& lighting);
    void drawModel(const Model& model, const glm::mat4& world, const DrawStyle& style = {});
    void drawMesh(const Mesh& mesh, const glm::mat4& world, const glm::vec3& color, const DrawStyle& style = {});
    void drawParticles(ParticleSystem& particles);
    void endFrame();

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
    Cubemap sky_;
    Mesh skyCube_;
    DebugLines debugLines_;
    glm::mat4 view_{1.0f};
    glm::mat4 projection_{1.0f};
    float viewportHeight_ = 1.0f;
};

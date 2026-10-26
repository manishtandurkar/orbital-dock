#include "render/Renderer.h"

#include "render/Camera.h"
#include "render/GlDebug.h"
#include "render/Model.h"
#include "render/ParticleSystem.h"
#include "render/Texture.h"

#include <glad/glad.h>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <string>

namespace {

constexpr int kDiffuseTextureUnit = 0;
constexpr int kSkyTextureUnit = 1;
constexpr float kSkyBrightness = 1.0f;

std::string pointLightUniform(int index, const char* field)
{
    return "uPointLights[" + std::to_string(index) + "]." + field;
}

}

Renderer::Renderer()
    : litShader_("shaders/blinn_phong.vert", "shaders/blinn_phong.frag")
    , skyboxShader_("shaders/skybox.vert", "shaders/skybox.frag")
    , lineShader_("shaders/debug_line.vert", "shaders/debug_line.frag")
    , particleShader_("shaders/particle.vert", "shaders/particle.frag")
    , sky_("textures/sky_")
    , skyCube_(Mesh::createUnitCube())
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    litShader_.use();
    litShader_.setInt("uDiffuseMap", kDiffuseTextureUnit);
    skyboxShader_.use();
    skyboxShader_.setInt("uSky", kSkyTextureUnit);
    gldebug::checkErrors("Renderer::Renderer");
}

void Renderer::clear(const glm::ivec2& framebufferSize)
{
    glViewport(0, 0, framebufferSize.x, framebufferSize.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::beginView(const Camera& camera, const glm::ivec2& viewportOrigin, const glm::ivec2& viewportSize, const Lighting& lighting)
{
    glViewport(viewportOrigin.x, viewportOrigin.y, viewportSize.x, viewportSize.y);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    viewportHeight_ = static_cast<float>(viewportSize.y);
    view_ = camera.viewMatrix();
    projection_ = camera.projectionMatrix();

    litShader_.use();
    litShader_.setMat4("uViewProjection", projection_ * view_);
    litShader_.setVec3("uViewPosition", camera.position());
    applyLighting(lighting);
}

void Renderer::applyLighting(const Lighting& lighting)
{
    sun_ = lighting.sun;
    litShader_.setVec3("uSun.direction", lighting.sun.direction);
    litShader_.setVec3("uSun.color", lighting.sun.color);
    litShader_.setFloat("uSun.intensity", lighting.sun.intensity);
    litShader_.setVec3("uAmbientColor", lighting.ambientColor);
    litShader_.setFloat("uExposure", lighting.exposure);

    const int count = std::min(static_cast<int>(lighting.pointLights.size()), Lighting::kMaxPointLights);
    litShader_.setInt("uPointLightCount", count);
    for (int i = 0; i < count; ++i) {
        const PointLight& light = lighting.pointLights[static_cast<size_t>(i)];
        litShader_.setVec3(pointLightUniform(i, "position"), light.position);
        litShader_.setVec3(pointLightUniform(i, "color"), light.color);
        litShader_.setFloat(pointLightUniform(i, "intensity"), light.intensity);
        litShader_.setFloat(pointLightUniform(i, "range"), light.range);
    }
}

void Renderer::setTransform(const glm::mat4& world)
{
    litShader_.setMat4("uModel", world);
    litShader_.setMat3("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(world))));
}

void Renderer::drawModel(const Model& model, const glm::mat4& world, const DrawStyle& style)
{
    litShader_.use();
    setTransform(world);
    litShader_.setVec3("uTint", style.tint);
    litShader_.setFloat("uEmissiveStrength", style.emissiveStrength);
    litShader_.setBool("uUnlit", style.unlit);
    litShader_.setFloat("uRimStrength", style.rimStrength);
    litShader_.setVec3("uRimColor", style.rimColor);

    for (const Model::Part& part : model.parts()) {
        const Material& material = part.material;
        litShader_.setVec3("uDiffuseColor", material.diffuseColor);
        litShader_.setVec3("uSpecularColor", material.specularColor);
        litShader_.setFloat("uShininess", material.shininess);
        litShader_.setVec3("uEmissiveColor", material.emissiveColor);
        const bool hasMap = material.diffuseMap != nullptr;
        litShader_.setBool("uUseDiffuseMap", hasMap);
        if (hasMap) {
            material.diffuseMap->bind(kDiffuseTextureUnit);
        }
        part.mesh.draw();
    }
}

void Renderer::drawMesh(const Mesh& mesh, const glm::mat4& world, const glm::vec3& color, const DrawStyle& style)
{
    litShader_.use();
    setTransform(world);
    litShader_.setVec3("uTint", style.tint);
    litShader_.setFloat("uEmissiveStrength", style.emissiveStrength);
    litShader_.setBool("uUnlit", style.unlit);
    litShader_.setFloat("uRimStrength", style.rimStrength);
    litShader_.setVec3("uRimColor", style.rimColor);
    litShader_.setVec3("uDiffuseColor", color);
    litShader_.setVec3("uSpecularColor", glm::vec3(0.3f));
    litShader_.setFloat("uShininess", 16.0f);
    litShader_.setVec3("uEmissiveColor", color);
    litShader_.setBool("uUseDiffuseMap", false);
    mesh.draw();
}

void Renderer::drawParticles(ParticleSystem& particles)
{
    if (particles.liveCount() == 0) {
        return;
    }
    particleShader_.use();
    particleShader_.setMat4("uViewProjection", projection_ * view_);
    particleShader_.setFloat("uViewportHeight", viewportHeight_);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    particles.draw();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::endView()
{
    drawDebugLines();
    drawSkybox();
    gldebug::checkErrors("Renderer::endView");
}

void Renderer::clearDebugLines()
{
    debugLines_.clear();
}

void Renderer::drawSkybox()
{
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    skyboxShader_.use();
    const glm::mat4 viewRotation = glm::mat4(glm::mat3(view_));
    skyboxShader_.setMat4("uViewRotationProjection", projection_ * viewRotation);
    skyboxShader_.setFloat("uBrightness", kSkyBrightness);
    skyboxShader_.setVec3("uSunDirection", glm::normalize(-sun_.direction));
    skyboxShader_.setVec3("uSunColor", sun_.color);
    sky_.bind(kSkyTextureUnit);
    skyCube_.draw();
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

void Renderer::drawDebugLines()
{
    if (debugLines_.empty()) {
        return;
    }
    debugLines_.upload();
    lineShader_.use();
    lineShader_.setMat4("uViewProjection", projection_ * view_);
    debugLines_.draw();
}

DebugLines& Renderer::debugLines()
{
    return debugLines_;
}

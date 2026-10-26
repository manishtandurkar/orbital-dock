#include "core/Game.h"

#include "render/GlDebug.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr float kCubeSpinDegreesPerSecond = 45.0f;

}

Game::Game()
    : window_(kWindowWidth, kWindowHeight, "Orbital Dock")
    , input_(window_.handle())
    , stateMachine_(GameState::Menu)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_MULTISAMPLE);

    litShader_ = std::make_unique<Shader>("shaders/blinn_phong.vert", "shaders/blinn_phong.frag");
    cube_ = std::make_unique<Mesh>(Mesh::createUnitCube());
    camera_.lookAt(glm::vec3(0.0f, 1.5f, 4.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Game::run()
{
    while (!window_.shouldClose()) {
        window_.pollEvents();
        input_.update();
        const float deltaSeconds = timer_.tick();
        update(deltaSeconds);
        if (!window_.isMinimized()) {
            render();
        }
        window_.swapBuffers();
    }
}

void Game::update(float deltaSeconds)
{
    if (input_.wasKeyPressed(GLFW_KEY_ESCAPE)) {
        window_.requestClose();
    }
    stateMachine_.advance(deltaSeconds);
    cubeAngle_ += kCubeSpinDegreesPerSecond * deltaSeconds;
}

void Game::render()
{
    const glm::ivec2 size = window_.framebufferSize();
    glViewport(0, 0, size.x, size.y);
    glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    camera_.setAspectRatio(window_.aspectRatio());
    const glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(cubeAngle_), glm::vec3(0.3f, 1.0f, 0.1f));

    litShader_->use();
    litShader_->setMat4("uModel", model);
    litShader_->setMat3("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    litShader_->setMat4("uViewProjection", camera_.viewProjectionMatrix());
    litShader_->setVec3("uViewPosition", camera_.position());
    litShader_->setFloat("uExposure", 1.0f);
    litShader_->setVec3("uAmbientColor", glm::vec3(0.08f));
    litShader_->setVec3("uSun.direction", glm::vec3(-0.5f, -0.6f, -0.4f));
    litShader_->setVec3("uSun.color", glm::vec3(1.0f, 0.95f, 0.85f));
    litShader_->setFloat("uSun.intensity", 2.0f);
    litShader_->setInt("uPointLightCount", 0);
    litShader_->setVec3("uDiffuseColor", glm::vec3(0.7f, 0.72f, 0.75f));
    litShader_->setVec3("uSpecularColor", glm::vec3(0.6f));
    litShader_->setFloat("uShininess", 48.0f);
    litShader_->setVec3("uEmissiveColor", glm::vec3(0.0f));
    litShader_->setFloat("uEmissiveStrength", 0.0f);
    litShader_->setVec3("uTint", glm::vec3(1.0f));
    litShader_->setBool("uUseDiffuseMap", false);
    litShader_->setBool("uUnlit", false);
    cube_->draw();

    gldebug::checkErrors("Game::render");
}

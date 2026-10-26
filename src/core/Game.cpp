#include "core/Game.h"

#include "gameplay/Station.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr float kOrbitDegreesPerSecond = 6.0f;

}

Game::Game()
    : window_(kWindowWidth, kWindowHeight, "Orbital Dock")
    , input_(window_.handle())
    , stateMachine_(GameState::Menu)
    , world_("world")
{
    renderer_ = std::make_unique<Renderer>();
    station_ = std::make_unique<Station>(world_, models_);
    station_->setArmDeployed(true);

    SceneNode& pod = world_.createChild("pod");
    pod.setModel(&models_.get("pod"));
    pod.setPosition({0.0f, 0.0f, 40.0f});

    const char* debrisNames[] = {"debris_rock_a", "debris_rock_b", "debris_panel", "debris_tank", "debris_beam"};
    for (int i = 0; i < 5; ++i) {
        SceneNode& debris = world_.createChild("debris");
        debris.setModel(&models_.get(debrisNames[i]));
        debris.setPosition({-12.0f + 6.0f * static_cast<float>(i), 10.0f, 50.0f});
        debris.setScale(glm::vec3(1.5f));
    }

    camera_.setPerspective(60.0f, 0.1f, 4000.0f);
}

Game::~Game() = default;

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
    station_->update(deltaSeconds, timer_.elapsedSeconds());
    orbitAngle_ += kOrbitDegreesPerSecond * deltaSeconds;
    const float radians = glm::radians(orbitAngle_);
    const glm::vec3 eye(std::sin(radians) * 70.0f, 25.0f, std::cos(radians) * 90.0f);
    camera_.lookAt(eye, glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

Lighting Game::buildLighting() const
{
    Lighting lighting;
    station_->appendLights(lighting.pointLights);
    return lighting;
}

void Game::render()
{
    camera_.setAspectRatio(window_.aspectRatio());
    renderer_->beginFrame(camera_, window_.framebufferSize(), buildLighting());
    world_.visit([this](const SceneNode& node, const glm::mat4& worldMatrix) {
        if (node.model() != nullptr) {
            renderer_->drawModel(*node.model(), worldMatrix, node.drawStyle());
        }
    });
    renderer_->endFrame();
}

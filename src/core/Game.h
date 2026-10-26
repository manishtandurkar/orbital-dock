#pragma once

#include "core/GameStateMachine.h"
#include "core/Input.h"
#include "core/Timer.h"
#include "core/Window.h"
#include "render/Camera.h"
#include "render/ModelLibrary.h"
#include "render/Renderer.h"
#include "scene/SceneNode.h"

#include <memory>

class Station;

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    void update(float deltaSeconds);
    void render();
    Lighting buildLighting() const;

    Window window_;
    Input input_;
    Timer timer_;
    GameStateMachine stateMachine_;
    Camera camera_;
    std::unique_ptr<Renderer> renderer_;
    ModelLibrary models_;
    SceneNode world_;
    std::unique_ptr<Station> station_;
    float orbitAngle_ = 0.0f;
};

#pragma once

#include "core/GameStateMachine.h"
#include "core/Input.h"
#include "core/Timer.h"
#include "core/Window.h"
#include "render/Camera.h"
#include "render/Mesh.h"
#include "render/Shader.h"

#include <memory>

class Game {
public:
    Game();

    void run();

private:
    void update(float deltaSeconds);
    void render();

    Window window_;
    Input input_;
    Timer timer_;
    GameStateMachine stateMachine_;
    Camera camera_;
    std::unique_ptr<Shader> litShader_;
    std::unique_ptr<Mesh> cube_;
    float cubeAngle_ = 0.0f;
};

#pragma once

#include "audio/AudioEngine.h"
#include "core/GameStateMachine.h"
#include "core/Input.h"
#include "core/Timer.h"
#include "core/Window.h"
#include "gameplay/ChaseCamera.h"
#include "gameplay/CollisionResolver.h"
#include "gameplay/EvaController.h"
#include "gameplay/Mission.h"
#include "physics/DockingCheck.h"
#include "render/Camera.h"
#include "render/ModelLibrary.h"
#include "render/Renderer.h"
#include "scene/SceneNode.h"
#include "ui/Hud.h"
#include "ui/Menu.h"

#include <memory>
#include <string>

class DebrisField;
class ImGuiLayer;
class ParticleSystem;
class Pod;
class Station;

class Game {
public:
    Game();
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void run();

private:
    void handleGlobalInput();
    void placePodOnDockingApproach();
    void update(float deltaSeconds);
    void updateFlight(float deltaSeconds);
    void updateDocking(float deltaSeconds);
    void updateEva(float deltaSeconds);
    void updateCinematicCamera(float deltaSeconds);
    void updateAudio();
    void updateCursorCapture();
    void updateParticles(float deltaSeconds);

    void changeState(GameState next);
    void onStateEntered(GameState from, GameState to);
    void resetMission();
    void dockPod();
    void handleImpacts();
    void showBanner(const std::string& text, float seconds);
    void applyMenuAction(MenuAction action);
    bool isPlayableState() const;

    void render();
    void renderScene();
    void renderDebugVolumes();
    void renderUi();
    Lighting buildLighting() const;
    HudData buildHudData() const;
    ScreenMarker projectToScreen(const glm::vec3& worldPoint) const;

    Window window_;
    Input input_;
    Timer timer_;
    GameStateMachine stateMachine_;
    Camera camera_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<ImGuiLayer> imgui_;
    std::unique_ptr<ParticleSystem> particles_;
    AudioEngine audio_;
    ModelLibrary models_;
    SceneNode world_;
    std::unique_ptr<Station> station_;
    std::unique_ptr<Pod> pod_;
    std::unique_ptr<DebrisField> debris_;
    const Model* toolModel_ = nullptr;

    ChaseCamera chaseCamera_;
    EvaController eva_;
    DockingCheck dockingCheck_;
    DockingStatus dockingStatus_;
    CollisionResolver collisions_;
    Mission mission_;
    Hud hud_;
    Menu menu_;

    bool alignmentAssist_ = true;
    bool showDebug_ = false;
    bool docked_ = false;
    float dockedSeconds_ = 0.0f;
    float winDelaySeconds_ = -1.0f;
    float damageFlash_ = 0.0f;
    float lastLockToneStep_ = 0.0f;
    float cinematicAngle_ = 0.0f;
    float panelSparkCooldown_ = 0.0f;
    std::string banner_;
    float bannerSeconds_ = 0.0f;
    int lastPointLightCount_ = 0;
};

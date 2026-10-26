#pragma once

#include "core/GameStateMachine.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <optional>
#include <string>
#include <vector>

struct DemoSnapshot {
    GameState state = GameState::Menu;
    glm::vec3 podPosition{0.0f};
    glm::vec3 podVelocity{0.0f};
    glm::vec3 podForward{0.0f, 0.0f, -1.0f};
    glm::vec3 podUp{0.0f, 1.0f, 0.0f};
    glm::vec3 podRight{1.0f, 0.0f, 0.0f};
    glm::vec3 podAngularVelocity{0.0f};
    float hullFraction = 1.0f;
    bool docked = false;
    bool cockpitView = false;
    glm::vec3 dockingTarget{0.0f};
    glm::vec3 dockingAxis{0.0f, 0.0f, 1.0f};
    glm::vec3 dockingUp{0.0f, 1.0f, 0.0f};
    float stationSpinRadiansPerSecond = 0.0f;
    int closingDebris = 0;
    bool debrisAhead = false;
    glm::vec3 debrisAheadPosition{0.0f};
    glm::vec3 cameraPosition{0.0f};
    glm::vec3 cameraForward{0.0f, 0.0f, -1.0f};
    glm::vec3 cameraRight{1.0f, 0.0f, 0.0f};
    glm::vec3 cameraUp{0.0f, 1.0f, 0.0f};
    glm::vec3 panelPosition{0.0f};
    bool canInteract = false;
    bool repairing = false;
    bool repaired = false;
};

struct DemoCommands {
    std::vector<int> keysDown;
    glm::vec2 mouseDelta{0.0f};
    bool startMission = false;
    bool quit = false;
    bool hideHud = false;
    std::string captionTitle;
    std::string captionText;
    float captionAlpha = 0.0f;
};

struct DemoShot {
    glm::vec3 eye{0.0f};
    glm::vec3 target{0.0f};
};

class DemoDirector {
public:
    DemoCommands update(const DemoSnapshot& snapshot, float deltaSeconds);
    std::optional<DemoShot> cameraShot(const glm::vec3& podPosition) const;

private:
    enum class Shot {
        None,
        StationReveal,
        DockingCloseUp
    };

    void trackState(const DemoSnapshot& snapshot, float deltaSeconds);
    void queueCaptions(const DemoSnapshot& snapshot);
    void showCaption(const std::string& title, const std::string& text);
    void chooseShot(const DemoSnapshot& snapshot);
    void flyPod(const DemoSnapshot& snapshot, DemoCommands& commands) const;
    void walkEva(const DemoSnapshot& snapshot, DemoCommands& commands) const;
    void fillCaption(DemoCommands& commands) const;
    float axialDistance(const DemoSnapshot& snapshot) const;

    GameState lastState_ = GameState::Menu;
    float stateSeconds_ = 0.0f;
    float totalSeconds_ = 0.0f;
    float lastHullFraction_ = 1.0f;
    bool impactCaptioned_ = false;
    bool revealCaptioned_ = false;
    bool dockingCaptioned_ = false;
    bool dockedCaptioned_ = false;
    bool repairCaptioned_ = false;
    Shot shot_ = Shot::None;
    std::string captionTitle_;
    std::string captionText_;
    float captionStartSeconds_ = -100.0f;
};

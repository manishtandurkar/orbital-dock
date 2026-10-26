#pragma once

#include "core/GameStateMachine.h"
#include "physics/DockingCheck.h"

#include <glm/vec2.hpp>

#include <string>

struct ScreenMarker {
    bool visible = false;
    glm::vec2 position{0.0f};
};

struct HudData {
    GameState state = GameState::Flight;
    float hullFraction = 1.0f;
    float speed = 0.0f;
    float distanceToPort = 0.0f;
    int closingDebris = 0;
    float damageFlash = 0.0f;
    bool alignmentAssist = true;
    bool cockpitView = false;
    DockingStatus docking;
    DockingTolerance tolerance;
    bool docked = false;
    ScreenMarker portMarker;
    ScreenMarker boresightMarker;
    float lifeSupportSeconds = 0.0f;
    float lifeSupportFraction = 1.0f;
    bool canInteract = false;
    bool repairing = false;
    float repairProgress = 0.0f;
    float distanceToPanel = 0.0f;
    std::string banner;
    float bannerAlpha = 0.0f;
    bool showDebug = false;
    float framesPerSecond = 0.0f;
    int pointLightCount = 0;
};

class Hud {
public:
    void draw(const HudData& data) const;

private:
    void drawFlightPanel(const HudData& data) const;
    void drawDockingPanel(const HudData& data) const;
    void drawDockingMarkers(const HudData& data) const;
    void drawEvaPanel(const HudData& data) const;
    void drawDamageFlash(const HudData& data) const;
    void drawBanner(const HudData& data) const;
    void drawDebug(const HudData& data) const;
};

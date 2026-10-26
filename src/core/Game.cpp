#include "core/Game.h"

#include "gameplay/DebrisField.h"
#include "gameplay/Pod.h"
#include "gameplay/Station.h"
#include "render/GlDebug.h"
#include "render/Model.h"
#include "render/ParticleSystem.h"
#include "ui/ImGuiLayer.h"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr float kFieldOfViewDegrees = 62.0f;
constexpr float kNearPlane = 0.05f;
constexpr float kFarPlane = 4000.0f;

constexpr glm::vec3 kPodStartPosition{0.0f, 10.0f, 330.0f};
constexpr float kDockingZoneEnterDistance = 90.0f;
constexpr float kDockingZoneExitDistance = 140.0f;
constexpr float kDockedSequenceSeconds = 3.2f;
constexpr float kWinDelaySeconds = 2.0f;
constexpr float kDamageFlashDecayPerSecond = 2.2f;
constexpr float kProximityWarningDistance = 30.0f;
constexpr float kCinematicDegreesPerSecond = 5.0f;
constexpr float kAxisGuideLength = 70.0f;
constexpr float kBannerFadeSeconds = 0.8f;
constexpr float kDebugApproachDistance = 20.0f;

const glm::vec3 kGuideColor(0.3f, 1.0f, 0.45f);
const glm::vec4 kImpactSparkColor(1.0f, 0.6f, 0.2f, 1.0f);
const glm::vec4 kPanelSparkColor(1.0f, 0.75f, 0.3f, 1.0f);
const glm::vec4 kArcSparkColor(0.55f, 0.8f, 1.0f, 1.0f);
const glm::vec4 kExhaustColor(0.35f, 0.6f, 1.0f, 0.8f);
const glm::vec4 kDockingPuffColor(0.8f, 0.85f, 0.9f, 0.6f);
constexpr glm::vec3 kPodNozzleOffset{0.0f, 0.0f, 2.8f};
constexpr float kPanelSparkIntervalSeconds = 0.35f;

}

Game::Game()
    : window_(kWindowWidth, kWindowHeight, "Orbital Dock")
    , input_(window_.handle())
    , stateMachine_(GameState::Menu)
    , world_("world")
{
    renderer_ = std::make_unique<Renderer>();
    imgui_ = std::make_unique<ImGuiLayer>(window_.handle());
    particles_ = std::make_unique<ParticleSystem>();

    station_ = std::make_unique<Station>(world_, models_);
    SceneNode& podNode = world_.createChild("pod");
    podNode.setModel(&models_.get("pod"));
    pod_ = std::make_unique<Pod>(podNode);
    debris_ = std::make_unique<DebrisField>(world_, models_);
    toolModel_ = &models_.get("repair_tool");

    camera_.setPerspective(kFieldOfViewDegrees, kNearPlane, kFarPlane);
    stateMachine_.setTransitionHandler([this](GameState from, GameState to) { onStateEntered(from, to); });

    resetMission();
    audio_.setLoop(Sound::AmbientHum, true, 0.35f);
}

Game::~Game() = default;

void Game::run()
{
    while (!window_.shouldClose()) {
        window_.pollEvents();
        input_.update();
        const float deltaSeconds = timer_.tick();
        handleGlobalInput();
        update(deltaSeconds);
        if (!window_.isMinimized()) {
            render();
        }
        window_.swapBuffers();
    }
}

bool Game::isPlayableState() const
{
    const GameState state = stateMachine_.current();
    return state == GameState::Flight || state == GameState::Docking || state == GameState::Eva;
}

void Game::handleGlobalInput()
{
    if (input_.wasKeyPressed(GLFW_KEY_ESCAPE)) {
        if (isPlayableState()) {
            stateMachine_.setPaused(!stateMachine_.isPaused());
            if (stateMachine_.isPaused()) {
                audio_.stopAllLoops();
            }
        } else if (stateMachine_.current() == GameState::Menu) {
            window_.requestClose();
        }
    }
    if (input_.wasKeyPressed(GLFW_KEY_F1)) {
        showDebug_ = !showDebug_;
    }
    if (stateMachine_.isPaused() || !isPlayableState()) {
        return;
    }
    if (input_.wasKeyPressed(GLFW_KEY_TAB)) {
        alignmentAssist_ = !alignmentAssist_;
    }
    if (input_.wasKeyPressed(GLFW_KEY_V) && stateMachine_.current() != GameState::Eva) {
        chaseCamera_.toggleView();
    }
    if (showDebug_ && input_.wasKeyPressed(GLFW_KEY_F2) && stateMachine_.current() != GameState::Eva && !docked_) {
        placePodOnDockingApproach();
    }
}

void Game::placePodOnDockingApproach()
{
    station_->deployArmImmediately();
    const glm::vec3 axis = station_->dockingAxisWorld();
    const glm::vec3 position = station_->dockingTargetWorld() + axis * kDebugApproachDistance;
    pod_->placeAt(position, glm::quatLookAt(-axis, station_->dockingUpWorld()));
    chaseCamera_.snapTo(pod_->position(), pod_->orientation());
}

void Game::changeState(GameState next)
{
    stateMachine_.transitionTo(next);
}

void Game::onStateEntered(GameState from, GameState to)
{
    switch (to) {
    case GameState::Menu:
        resetMission();
        break;
    case GameState::Flight:
        if (from == GameState::Menu || from == GameState::Win || from == GameState::Lose) {
            showBanner("Reach the station. Avoid the debris.", 4.0f);
        }
        break;
    case GameState::Docking:
        station_->setArmDeployed(true);
        dockingCheck_.reset();
        showBanner("Docking zone: match the port axis and roll", 4.0f);
        break;
    case GameState::Eva:
        eva_.reset();
        mission_.startEva();
        input_.resetMouseDelta();
        showBanner("EVA: reach the damaged panel and repair it", 4.0f);
        break;
    case GameState::Win:
        audio_.stopAllLoops();
        break;
    case GameState::Lose:
        audio_.stopAllLoops();
        audio_.playOneShot(Sound::Impact, 1.0f);
        break;
    }
    input_.resetMouseDelta();
}

void Game::resetMission()
{
    if (pod_->node().parent() != &world_) {
        pod_->node().reparentTo(world_, false);
    }
    pod_->node().setVisible(true);
    pod_->reset(kPodStartPosition, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    station_->reset();
    debris_->reset();
    dockingCheck_.reset();
    dockingStatus_ = DockingStatus{};
    mission_.reset();
    eva_.reset();
    chaseCamera_.snapTo(pod_->position(), pod_->orientation());
    docked_ = false;
    dockedSeconds_ = 0.0f;
    winDelaySeconds_ = -1.0f;
    damageFlash_ = 0.0f;
    panelSparkCooldown_ = 0.0f;
    particles_->clear();
    lastLockToneStep_ = 0.0f;
    banner_.clear();
    bannerSeconds_ = 0.0f;
}

void Game::showBanner(const std::string& text, float seconds)
{
    banner_ = text;
    bannerSeconds_ = seconds;
}

void Game::update(float deltaSeconds)
{
    updateCursorCapture();
    if (stateMachine_.isPaused()) {
        return;
    }
    stateMachine_.advance(deltaSeconds);
    station_->update(deltaSeconds, timer_.elapsedSeconds());
    damageFlash_ = std::max(damageFlash_ - kDamageFlashDecayPerSecond * deltaSeconds, 0.0f);
    bannerSeconds_ = std::max(bannerSeconds_ - deltaSeconds, 0.0f);

    switch (stateMachine_.current()) {
    case GameState::Menu:
    case GameState::Win:
    case GameState::Lose:
        debris_->update(deltaSeconds, pod_->position(), glm::vec3(0.0f), pod_->forward(), false);
        updateCinematicCamera(deltaSeconds);
        break;
    case GameState::Flight:
        updateFlight(deltaSeconds);
        break;
    case GameState::Docking:
        updateFlight(deltaSeconds);
        if (stateMachine_.current() == GameState::Docking) {
            updateDocking(deltaSeconds);
        }
        break;
    case GameState::Eva:
        updateEva(deltaSeconds);
        break;
    }
    updateParticles(deltaSeconds);
    updateAudio();
}

void Game::updateParticles(float deltaSeconds)
{
    particles_->update(deltaSeconds);

    panelSparkCooldown_ -= deltaSeconds;
    if (!mission_.isRepaired() && panelSparkCooldown_ <= 0.0f) {
        panelSparkCooldown_ = kPanelSparkIntervalSeconds;
        const SceneNode& panel = station_->repairPanel();
        ParticleBurst burst;
        burst.position = panel.localToWorldPoint({0.1f, -0.1f, 0.1f});
        burst.baseVelocity = panel.localToWorldDirection({0.0f, -0.6f, 1.2f});
        burst.spreadSpeed = 1.6f;
        burst.color = kPanelSparkColor;
        burst.lifeSeconds = 0.5f;
        burst.size = 0.07f;
        burst.count = 16;
        particles_->emit(burst);
    }

    const GameState state = stateMachine_.current();
    const bool flying = (state == GameState::Flight || state == GameState::Docking) && !docked_;
    if (flying && pod_->thrusterLevel() > 0.1f) {
        ParticleBurst exhaust;
        exhaust.position = pod_->node().localToWorldPoint(kPodNozzleOffset);
        exhaust.baseVelocity = pod_->velocity() - pod_->forward() * 9.0f;
        exhaust.spreadSpeed = 1.2f;
        exhaust.color = kExhaustColor * pod_->thrusterLevel();
        exhaust.lifeSeconds = 0.35f;
        exhaust.size = 0.18f;
        exhaust.count = 3;
        particles_->emit(exhaust);
    }

    if (state == GameState::Eva && eva_.isRepairing()) {
        ParticleBurst arc;
        arc.position = glm::vec3(eva_.toolWorldMatrix(station_->deck(), timer_.elapsedSeconds()) * glm::vec4(0.0f, 0.0f, -0.36f, 1.0f));
        arc.spreadSpeed = 1.5f;
        arc.color = kArcSparkColor;
        arc.lifeSeconds = 0.3f;
        arc.size = 0.03f;
        arc.count = 4;
        particles_->emit(arc);
    }
}

void Game::updateCursorCapture()
{
    const bool capture = isPlayableState() && !stateMachine_.isPaused();
    if (capture != window_.isCursorCaptured()) {
        window_.setCursorCaptured(capture);
        input_.resetMouseDelta();
    }
}

void Game::updateFlight(float deltaSeconds)
{
    const bool inDocking = stateMachine_.current() == GameState::Docking;
    const PodControls controls = docked_ ? PodControls{} : PodControls::fromInput(input_);
    const glm::vec3 matchSpin = inDocking && alignmentAssist_ ? station_->spinAxisWorld() * station_->spinRadiansPerSecond()
                                                              : glm::vec3(0.0f);
    pod_->update(controls, deltaSeconds, matchSpin);
    debris_->update(deltaSeconds, pod_->position(), pod_->velocity(), pod_->forward(), !inDocking);
    mission_.updateFlight(deltaSeconds);
    handleImpacts();

    if (pod_->isDestroyed()) {
        mission_.fail(LossReason::HullDestroyed);
        changeState(GameState::Lose);
        return;
    }

    const float distance = glm::distance(pod_->worldPosition(), station_->dockingTargetWorld());
    if (!inDocking && distance < kDockingZoneEnterDistance) {
        changeState(GameState::Docking);
    } else if (inDocking && !docked_ && distance > kDockingZoneExitDistance) {
        changeState(GameState::Flight);
    }

    pod_->node().setVisible(chaseCamera_.view() == ChaseCamera::View::Chase);
    pod_->node().drawStyle().emissiveStrength = 0.5f + pod_->thrusterLevel() * 1.5f;
    chaseCamera_.update(camera_, pod_->worldPosition(), pod_->worldOrientation(), deltaSeconds, timer_.elapsedSeconds());
}

void Game::handleImpacts()
{
    const std::vector<ImpactEvent> impacts = collisions_.resolve(*pod_, *debris_, *station_);
    for (const ImpactEvent& impact : impacts) {
        pod_->applyDamage(impact.damage);
        mission_.recordImpact(impact.damage);
        chaseCamera_.addShake(std::min(0.35f + impact.damage / 30.0f, 1.0f));
        damageFlash_ = std::min(damageFlash_ + 0.5f + impact.damage / 30.0f, 1.0f);
        audio_.playOneShot(Sound::Impact, std::min(0.4f + impact.damage / 25.0f, 1.0f));
        ParticleBurst sparks;
        sparks.position = impact.point;
        sparks.baseVelocity = pod_->velocity();
        sparks.spreadSpeed = 4.0f + impact.impactSpeed * 0.3f;
        sparks.color = kImpactSparkColor;
        sparks.lifeSeconds = 0.8f;
        sparks.size = 0.12f;
        sparks.count = 30 + static_cast<int>(impact.damage * 2.0f);
        particles_->emit(sparks);
    }
}

void Game::updateDocking(float deltaSeconds)
{
    if (docked_) {
        dockedSeconds_ += deltaSeconds;
        if (dockedSeconds_ >= kDockedSequenceSeconds) {
            changeState(GameState::Eva);
        }
        return;
    }

    DockingInputs inputs;
    inputs.podPosition = pod_->position();
    inputs.podVelocity = pod_->velocity();
    inputs.podForward = pod_->forward();
    inputs.podUp = pod_->up();
    inputs.targetPosition = station_->dockingTargetWorld();
    inputs.targetVelocity = station_->pointVelocity(inputs.targetPosition);
    inputs.portAxis = station_->dockingAxisWorld();
    inputs.portUp = station_->dockingUpWorld();
    if (!station_->isArmDeployed()) {
        dockingCheck_.reset();
    }
    dockingStatus_ = dockingCheck_.evaluate(inputs, station_->isArmDeployed() ? deltaSeconds : 0.0f);

    const float toneStep = std::floor(dockingStatus_.holdProgress * 4.0f);
    if (toneStep > lastLockToneStep_) {
        audio_.playOneShot(Sound::LockTone, 0.5f);
    }
    lastLockToneStep_ = toneStep;

    if (dockingStatus_.locked) {
        dockPod();
    }
}

void Game::dockPod()
{
    docked_ = true;
    dockedSeconds_ = 0.0f;
    pod_->setDocked(true);
    pod_->node().reparentTo(station_->dockingPort(), true);
    pod_->node().setLocalTransform(station_->dockedPodLocalTransform());
    station_->setClampsClosed(true);
    chaseCamera_.addShake(0.45f);
    audio_.playOneShot(Sound::DockingClunk, 1.0f);
    ParticleBurst puff;
    puff.position = station_->dockingPort().localToWorldPoint({0.0f, 0.0f, 1.2f});
    puff.spreadSpeed = 2.5f;
    puff.color = kDockingPuffColor;
    puff.lifeSeconds = 1.2f;
    puff.size = 0.3f;
    puff.count = 60;
    particles_->emit(puff);
    showBanner("Docked. Clamps engaged.", 3.0f);
}

void Game::updateEva(float deltaSeconds)
{
    const SceneNode& panel = station_->repairPanel();
    eva_.update(input_, deltaSeconds, panel.position(), mission_.isRepaired());
    mission_.updateEva(deltaSeconds);
    eva_.applyToCamera(camera_, station_->deck());
    pod_->node().setVisible(true);

    if (eva_.repairJustCompleted()) {
        mission_.markRepaired();
        station_->setPanelRepaired(true);
        station_->setAlarmActive(false);
        audio_.playOneShot(Sound::RepairComplete, 1.0f);
        showBanner("Panel repaired. Life support restored!", 3.0f);
        winDelaySeconds_ = kWinDelaySeconds;
    }
    if (winDelaySeconds_ >= 0.0f) {
        winDelaySeconds_ -= deltaSeconds;
        if (winDelaySeconds_ < 0.0f) {
            changeState(GameState::Win);
        }
        return;
    }
    if (mission_.isLifeSupportDepleted()) {
        mission_.fail(LossReason::LifeSupportDepleted);
        changeState(GameState::Lose);
    }
}

void Game::updateCinematicCamera(float deltaSeconds)
{
    cinematicAngle_ += kCinematicDegreesPerSecond * deltaSeconds;
    const float radians = glm::radians(cinematicAngle_);
    const glm::vec3 eye(std::sin(radians) * 85.0f, 22.0f + std::sin(radians * 0.7f) * 10.0f, 40.0f + std::cos(radians) * 85.0f);
    camera_.lookAt(eye, glm::vec3(0.0f, 0.0f, 8.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Game::updateAudio()
{
    const GameState state = stateMachine_.current();
    const bool flying = state == GameState::Flight || state == GameState::Docking;
    audio_.setLoop(Sound::AmbientHum, true, state == GameState::Eva ? 0.6f : 0.3f);
    audio_.setLoop(Sound::Thruster, flying && pod_->thrusterLevel() > 0.05f, pod_->thrusterLevel() * 0.55f);

    const bool hullAlarm = flying && pod_->hullFraction() < 0.3f;
    const bool oxygenAlarm = state == GameState::Eva && !mission_.isRepaired() && mission_.lifeSupportFraction() < 0.2f;
    audio_.setLoop(Sound::HullAlarm, hullAlarm || oxygenAlarm, 0.45f);
    audio_.setLoop(Sound::RepairWeld, state == GameState::Eva && eva_.isRepairing(), 0.6f);
}

void Game::applyMenuAction(MenuAction action)
{
    switch (action) {
    case MenuAction::Start:
    case MenuAction::Restart:
        resetMission();
        changeState(GameState::Flight);
        break;
    case MenuAction::Resume:
        stateMachine_.setPaused(false);
        break;
    case MenuAction::MainMenu:
        changeState(GameState::Menu);
        break;
    case MenuAction::Quit:
        window_.requestClose();
        break;
    case MenuAction::None:
        break;
    }
}

Lighting Game::buildLighting() const
{
    Lighting lighting;
    station_->appendLights(lighting.pointLights);
    const GameState state = stateMachine_.current();
    if (state == GameState::Flight || state == GameState::Docking) {
        lighting.pointLights.push_back({pod_->worldPosition() + pod_->forward() * 4.0f, {0.9f, 0.95f, 1.0f}, 6.0f, 45.0f});
    }
    if (state == GameState::Eva && eva_.isRepairing()) {
        const glm::vec3 tip = glm::vec3(eva_.toolWorldMatrix(station_->deck(), timer_.elapsedSeconds())
            * glm::vec4(0.0f, 0.0f, -0.35f, 1.0f));
        const float flicker = 0.6f + 0.4f * static_cast<float>(std::sin(timer_.elapsedSeconds() * 53.0));
        lighting.pointLights.push_back({tip, {0.5f, 0.75f, 1.0f}, 5.0f * flicker, 6.0f});
    }
    if (static_cast<int>(lighting.pointLights.size()) > Lighting::kMaxPointLights) {
        lighting.pointLights.resize(Lighting::kMaxPointLights);
    }
    return lighting;
}

void Game::render()
{
    camera_.setAspectRatio(window_.aspectRatio());
    renderScene();
    renderUi();
}

void Game::renderScene()
{
    const Lighting lighting = buildLighting();
    lastPointLightCount_ = static_cast<int>(lighting.pointLights.size());
    renderer_->beginFrame(camera_, window_.framebufferSize(), lighting);

    world_.visit([this](const SceneNode& node, const glm::mat4& worldMatrix) {
        if (node.model() != nullptr) {
            renderer_->drawModel(*node.model(), worldMatrix, node.drawStyle());
        }
    });

    if (stateMachine_.current() == GameState::Eva) {
        DrawStyle toolStyle;
        toolStyle.emissiveStrength = eva_.isRepairing() ? 3.0f : 0.6f;
        renderer_->drawModel(*toolModel_, eva_.toolWorldMatrix(station_->deck(), timer_.elapsedSeconds()), toolStyle);
    }

    if (stateMachine_.current() == GameState::Docking && alignmentAssist_ && !docked_) {
        const glm::vec3 start = station_->dockingTargetWorld();
        renderer_->debugLines().addLine(start, start + station_->dockingAxisWorld() * kAxisGuideLength, kGuideColor);
    }
    if (showDebug_) {
        renderDebugVolumes();
    }
    renderer_->endFrame();
    renderer_->drawParticles(*particles_);
}

void Game::renderDebugVolumes()
{
    DebugLines& lines = renderer_->debugLines();
    const glm::vec3 stationColor(1.0f, 0.8f, 0.1f);
    for (const StationCollider& collider : station_->colliders()) {
        lines.addBox(collider.node->worldMatrix(), collider.localBox.min, collider.localBox.max, stationColor);
    }
    for (const Sphere& sphere : station_->worldSphereColliders()) {
        lines.addSphere(sphere.center, sphere.radius, stationColor);
    }
    lines.addSphere(pod_->worldPosition(), Pod::kCollisionRadius, glm::vec3(0.2f, 0.8f, 1.0f));
    for (const Debris& debris : debris_->pieces()) {
        if (debris.isActive()) {
            const glm::vec3 color = debris.state() == DebrisState::Tumbling ? glm::vec3(1.0f, 0.2f, 0.2f) : glm::vec3(0.6f, 0.6f, 0.6f);
            lines.addSphere(debris.position(), debris.radius(), color);
        }
    }
    const glm::vec3 target = station_->dockingTargetWorld();
    lines.addSphere(target, dockingCheck_.tolerance().maxDistance, kGuideColor);
}

ScreenMarker Game::projectToScreen(const glm::vec3& worldPoint) const
{
    ScreenMarker marker;
    const glm::vec4 clip = camera_.viewProjectionMatrix() * glm::vec4(worldPoint, 1.0f);
    if (clip.w <= 0.01f) {
        return marker;
    }
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (std::abs(ndc.x) > 1.2f || std::abs(ndc.y) > 1.2f) {
        return marker;
    }
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    marker.visible = true;
    marker.position = {(ndc.x * 0.5f + 0.5f) * display.x, (0.5f - ndc.y * 0.5f) * display.y};
    return marker;
}

HudData Game::buildHudData() const
{
    HudData data;
    data.state = stateMachine_.current();
    data.hullFraction = pod_->hullFraction();
    data.speed = pod_->speed();
    data.distanceToPort = glm::distance(pod_->worldPosition(), station_->dockingTargetWorld());
    data.closingDebris = debris_->countClosingOn(pod_->position(), pod_->velocity(), kProximityWarningDistance);
    data.damageFlash = damageFlash_;
    data.alignmentAssist = alignmentAssist_;
    data.cockpitView = chaseCamera_.view() == ChaseCamera::View::Cockpit;
    data.docking = dockingStatus_;
    data.tolerance = dockingCheck_.tolerance();
    data.docked = docked_;
    data.portMarker = projectToScreen(station_->dockingTargetWorld());
    data.boresightMarker = projectToScreen(pod_->worldPosition() + pod_->forward() * std::max(data.distanceToPort, 5.0f));
    data.lifeSupportSeconds = mission_.lifeSupportSeconds();
    data.lifeSupportFraction = mission_.lifeSupportFraction();
    data.canInteract = eva_.canInteract();
    data.repairing = eva_.isRepairing();
    data.repairProgress = eva_.repairProgress();
    data.distanceToPanel = eva_.distanceToPanel();
    data.banner = banner_;
    data.bannerAlpha = std::min(bannerSeconds_ / kBannerFadeSeconds, 1.0f);
    data.showDebug = showDebug_;
    data.framesPerSecond = timer_.framesPerSecond();
    data.pointLightCount = lastPointLightCount_;
    return data;
}

void Game::renderUi()
{
    imgui_->beginFrame();
    MenuAction action = MenuAction::None;
    switch (stateMachine_.current()) {
    case GameState::Menu:
        action = menu_.drawMainMenu(audio_.isAvailable());
        break;
    case GameState::Win:
    case GameState::Lose: {
        MissionSummary summary;
        summary.won = stateMachine_.current() == GameState::Win;
        summary.message = summary.won ? "The hull panel is sealed and the station's life support is stable. Great flying!"
                                      : mission_.lossMessage();
        summary.missionSeconds = mission_.missionSeconds();
        summary.hullFraction = pod_->hullFraction();
        summary.impacts = mission_.impactCount();
        summary.lifeSupportLeftSeconds = mission_.lifeSupportSeconds();
        action = menu_.drawEndScreen(summary);
        break;
    }
    default:
        HudData hudData = buildHudData();
        hudData.alignmentAssist = alignmentAssist_ && !stateMachine_.isPaused();
        hud_.draw(hudData);
        if (stateMachine_.isPaused()) {
            action = menu_.drawPauseMenu();
        }
        break;
    }
    if (showDebug_ && !isPlayableState()) {
        HudData debugData = buildHudData();
        debugData.state = stateMachine_.current();
        hud_.draw(debugData);
    }
    imgui_->endFrame();
    gldebug::checkErrors("Game::renderUi");
    applyMenuAction(action);
}

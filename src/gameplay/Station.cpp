#include "gameplay/Station.h"

#include "render/ModelLibrary.h"
#include "scene/SceneNode.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace {

constexpr glm::vec3 kSpinAxisLocal{0.0f, 0.0f, 1.0f};
constexpr glm::vec3 kXAxis{1.0f, 0.0f, 0.0f};
constexpr glm::vec3 kZAxis{0.0f, 0.0f, 1.0f};

constexpr float kArmBaseZ = 13.0f;
constexpr float kArmLength = 10.0f;
constexpr float kArmStowedDegrees = -75.0f;
constexpr float kArmDeployedDegrees = 0.0f;
constexpr float kArmDegreesPerSecond = 18.0f;

constexpr float kClampRadius = 2.6f;
constexpr float kClampZ = 0.4f;
constexpr float kClampOpenDegrees = -40.0f;
constexpr float kClampClosedDegrees = 35.0f;
constexpr float kClampDegreesPerSecond = 70.0f;

constexpr float kDockedPodOffset = 4.4f;
constexpr float kPortStopPlaneZ = 2.3f;

constexpr float kDeckTop = 6.6f;
constexpr glm::vec3 kRepairPanelPosition{0.0f, kDeckTop + 1.4f, -10.47f};
constexpr glm::vec3 kWarningLightPosition{0.0f, kDeckTop + 3.3f, -11.1f};
constexpr glm::vec3 kLampHeadOffset{0.0f, 1.95f, -0.4f};
constexpr glm::vec3 kPortGuideLightOffset{0.0f, 2.7f, 1.4f};

constexpr float kRingInnerRadius = 31.4f;
constexpr float kRingOuterRadius = 36.6f;
constexpr float kRingHalfThickness = 2.6f;
constexpr int kRingColliderSegments = 24;

constexpr float kWarningBlinkRadiansPerSecond = 7.0f;

float approach(float current, float target, float maxStep)
{
    if (current < target) {
        return std::min(current + maxStep, target);
    }
    return std::max(current - maxStep, target);
}

glm::quat rotationAboutZ(float degrees)
{
    return glm::angleAxis(glm::radians(degrees), kZAxis);
}

glm::quat rotationAboutX(float degrees)
{
    return glm::angleAxis(glm::radians(degrees), kXAxis);
}

}

Station::Station(SceneNode& worldRoot, ModelLibrary& models)
{
    root_ = &worldRoot.createChild("station");
    buildHierarchy(models);
    buildColliders();
    reset();
}

void Station::buildHierarchy(ModelLibrary& models)
{
    hub_ = &root_->createChild("hub");
    hub_->setModel(&models.get("station_hub"));

    ring_ = &root_->createChild("ring");
    ring_->setModel(&models.get("station_ring"));

    solar_ = &root_->createChild("solar");
    solar_->setModel(&models.get("station_solar"));

    arm_ = &root_->createChild("docking_arm");
    arm_->setModel(&models.get("docking_arm"));
    arm_->setPosition({0.0f, 0.0f, kArmBaseZ});

    port_ = &arm_->createChild("docking_port");
    port_->setModel(&models.get("docking_port"));
    port_->setPosition({0.0f, 0.0f, kArmLength});

    const Model& clampModel = models.get("docking_clamp");
    for (size_t i = 0; i < clamps_.size(); ++i) {
        clamps_[i] = &port_->createChild("clamp_" + std::to_string(i));
        clamps_[i]->setModel(&clampModel);
    }

    deck_ = &root_->createChild("eva_deck");
    deck_->setModel(&models.get("eva_deck"));

    const Model& panelModel = models.get("hull_panel");
    const float panelSpacing = 3.0f;
    for (int i = 0; i < 6; ++i) {
        SceneNode& panel = deck_->createChild("hull_panel_" + std::to_string(i));
        panel.setModel(&panelModel);
        panel.setPosition({0.0f, kDeckTop, -8.0f + panelSpacing * static_cast<float>(i)});
    }

    repairPanelDamaged_ = &deck_->createChild("repair_panel_damaged");
    repairPanelDamaged_->setModel(&models.get("repair_panel_damaged"));
    repairPanelDamaged_->setPosition(kRepairPanelPosition);

    repairPanelFixed_ = &deck_->createChild("repair_panel_fixed");
    repairPanelFixed_->setModel(&models.get("repair_panel_fixed"));
    repairPanelFixed_->setPosition(kRepairPanelPosition);

    warningLight_ = &deck_->createChild("warning_light");
    warningLight_->setModel(&models.get("warning_light"));
    warningLight_->setPosition(kWarningLightPosition);

    const Model& floodlightModel = models.get("floodlight");
    const std::array<glm::vec3, 3> floodlightPositions = {{
        {-1.9f, kDeckTop, 9.0f},
        {1.9f, kDeckTop, 1.0f},
        {-1.9f, kDeckTop, -6.5f},
    }};
    for (size_t i = 0; i < floodlightPositions.size(); ++i) {
        SceneNode& lamp = deck_->createChild("floodlight_" + std::to_string(i));
        lamp.setModel(&floodlightModel);
        lamp.setPosition(floodlightPositions[i]);
        floodlights_.push_back(&lamp);
    }
}

void Station::buildColliders()
{
    colliders_.push_back({hub_, {{-6.0f, -4.2f, -13.0f}, {6.0f, 4.2f, 13.0f}}});
    colliders_.push_back({hub_, {{-4.2f, -6.0f, -13.0f}, {4.2f, 6.0f, 13.0f}}});
    colliders_.push_back({deck_, {{-2.2f, 6.0f, -11.7f}, {2.2f, 7.6f, 10.8f}}});
    colliders_.push_back({deck_, {{-2.2f, 6.6f, -11.7f}, {2.2f, 9.9f, -10.5f}}});
    colliders_.push_back({solar_, {{4.0f, -0.3f, -24.5f}, {40.0f, 0.3f, -9.5f}}});
    colliders_.push_back({solar_, {{-40.0f, -0.3f, -24.5f}, {-4.0f, 0.3f, -9.5f}}});
    colliders_.push_back({arm_, {{-2.0f, -0.9f, 0.0f}, {2.0f, 0.9f, 1.2f}}});
    colliders_.push_back({arm_, {{-0.9f, -0.9f, 0.0f}, {0.9f, 0.9f, kArmLength - 0.8f}}});
    colliders_.push_back({port_, {{-2.3f, -2.3f, -0.8f}, {2.3f, 2.3f, kPortStopPlaneZ}}});

    for (int segment = 0; segment < kRingColliderSegments; ++segment) {
        const float a0 = glm::two_pi<float>() * static_cast<float>(segment) / kRingColliderSegments;
        const float a1 = glm::two_pi<float>() * static_cast<float>(segment + 1) / kRingColliderSegments;
        const float middle = (a0 + a1) * 0.5f;
        glm::vec2 lowest(INFINITY);
        glm::vec2 highest(-INFINITY);
        for (const float angle : {a0, a1, middle}) {
            for (const float radius : {kRingInnerRadius, kRingOuterRadius}) {
                const glm::vec2 point(std::cos(angle) * radius, std::sin(angle) * radius);
                lowest = glm::min(lowest, point);
                highest = glm::max(highest, point);
            }
        }
        colliders_.push_back({ring_, {{lowest.x, lowest.y, -kRingHalfThickness}, {highest.x, highest.y, kRingHalfThickness}}});
    }

    for (int spoke = 0; spoke < 4; ++spoke) {
        const float angle = glm::radians(45.0f + 90.0f * static_cast<float>(spoke));
        const glm::vec3 direction(std::cos(angle), std::sin(angle), 0.0f);
        for (float radius = 8.0f; radius < kRingInnerRadius; radius += 2.5f) {
            sphereColliders_.emplace_back(ring_, Sphere{direction * radius, 1.2f});
        }
    }
}

void Station::reset()
{
    spinAngleDegrees_ = 0.0f;
    armAngleDegrees_ = kArmStowedDegrees;
    armDeployed_ = false;
    clampAngleDegrees_ = kClampOpenDegrees;
    clampsClosed_ = false;
    alarmActive_ = true;
    setPanelRepaired(false);
    root_->setRotation(rotationAboutZ(spinAngleDegrees_));
    updateArm(0.0f);
    updateClamps(0.0f);
}

void Station::update(float deltaSeconds, double elapsedSeconds)
{
    spinAngleDegrees_ = std::fmod(spinAngleDegrees_ + kSpinDegreesPerSecond * deltaSeconds, 360.0f);
    root_->setRotation(rotationAboutZ(spinAngleDegrees_));
    updateArm(deltaSeconds);
    updateClamps(deltaSeconds);
    updateLights(elapsedSeconds);
}

void Station::updateArm(float deltaSeconds)
{
    const float target = armDeployed_ ? kArmDeployedDegrees : kArmStowedDegrees;
    armAngleDegrees_ = approach(armAngleDegrees_, target, kArmDegreesPerSecond * deltaSeconds);
    arm_->setRotation(rotationAboutX(armAngleDegrees_));
}

void Station::updateClamps(float deltaSeconds)
{
    const float target = clampsClosed_ ? kClampClosedDegrees : kClampOpenDegrees;
    clampAngleDegrees_ = approach(clampAngleDegrees_, target, kClampDegreesPerSecond * deltaSeconds);
    for (size_t i = 0; i < clamps_.size(); ++i) {
        const glm::quat around = rotationAboutZ(90.0f * static_cast<float>(i));
        clamps_[i]->setPosition(around * glm::vec3(0.0f, kClampRadius, kClampZ));
        clamps_[i]->setRotation(around * rotationAboutX(clampAngleDegrees_));
    }
}

void Station::updateLights(double elapsedSeconds)
{
    const float phase = static_cast<float>(std::sin(elapsedSeconds * kWarningBlinkRadiansPerSecond));
    warningLevel_ = alarmActive_ ? glm::smoothstep(-0.2f, 0.4f, phase) : 0.0f;
    warningLight_->drawStyle().emissiveStrength = 0.15f + warningLevel_ * 2.0f;

    if (!panelRepaired_) {
        const float flicker = static_cast<float>(std::sin(elapsedSeconds * 37.0) * std::sin(elapsedSeconds * 23.0));
        repairPanelDamaged_->drawStyle().emissiveStrength = 0.4f + std::max(flicker, 0.0f) * 2.5f;
    }
}

void Station::setArmDeployed(bool deployed)
{
    armDeployed_ = deployed;
}

void Station::deployArmImmediately()
{
    armDeployed_ = true;
    armAngleDegrees_ = kArmDeployedDegrees;
    updateArm(0.0f);
}

bool Station::isArmDeployed() const
{
    return std::abs(armAngleDegrees_ - kArmDeployedDegrees) < 0.01f && armDeployed_;
}

void Station::setClampsClosed(bool closed)
{
    clampsClosed_ = closed;
}

bool Station::areClampsClosed() const
{
    return clampsClosed_ && std::abs(clampAngleDegrees_ - kClampClosedDegrees) < 0.01f;
}

void Station::setPanelRepaired(bool repaired)
{
    panelRepaired_ = repaired;
    repairPanelDamaged_->setVisible(!repaired);
    repairPanelFixed_->setVisible(repaired);
}

void Station::setAlarmActive(bool active)
{
    alarmActive_ = active;
}

SceneNode& Station::root()
{
    return *root_;
}

SceneNode& Station::dockingPort()
{
    return *port_;
}

SceneNode& Station::deck()
{
    return *deck_;
}

const SceneNode& Station::repairPanel() const
{
    return *repairPanelDamaged_;
}

glm::vec3 Station::spinAxisWorld() const
{
    return root_->localToWorldDirection(kSpinAxisLocal);
}

float Station::spinRadiansPerSecond() const
{
    return glm::radians(kSpinDegreesPerSecond);
}

glm::vec3 Station::pointVelocity(const glm::vec3& worldPoint) const
{
    const glm::vec3 angularVelocity = spinAxisWorld() * spinRadiansPerSecond();
    return glm::cross(angularVelocity, worldPoint - root_->worldPosition());
}

glm::vec3 Station::dockingTargetWorld() const
{
    return port_->localToWorldPoint({0.0f, 0.0f, kDockedPodOffset});
}

glm::vec3 Station::dockingAxisWorld() const
{
    return port_->localToWorldDirection(kZAxis);
}

glm::vec3 Station::dockingUpWorld() const
{
    return port_->localToWorldDirection({0.0f, 1.0f, 0.0f});
}

glm::mat4 Station::dockedPodLocalTransform() const
{
    return glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, kDockedPodOffset));
}

const std::vector<StationCollider>& Station::colliders() const
{
    return colliders_;
}

std::vector<Sphere> Station::worldSphereColliders() const
{
    std::vector<Sphere> spheres;
    spheres.reserve(sphereColliders_.size());
    for (const auto& [node, sphere] : sphereColliders_) {
        spheres.push_back({node->localToWorldPoint(sphere.center), sphere.radius});
    }
    return spheres;
}

void Station::appendLights(std::vector<PointLight>& lights) const
{
    for (const SceneNode* lamp : floodlights_) {
        lights.push_back({lamp->localToWorldPoint(kLampHeadOffset), {1.0f, 0.93f, 0.8f}, 3.2f, 15.0f});
    }
    if (warningLevel_ > 0.01f) {
        lights.push_back({warningLight_->localToWorldPoint({0.0f, 0.2f, 0.4f}), {1.0f, 0.15f, 0.08f}, 9.0f * warningLevel_, 14.0f});
    }
    lights.push_back({port_->localToWorldPoint(kPortGuideLightOffset), {0.3f, 1.0f, 0.4f}, 4.0f, 14.0f});
}

float Station::warningLightLevel() const
{
    return warningLevel_;
}

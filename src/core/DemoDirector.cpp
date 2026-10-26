#include "core/DemoDirector.h"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

namespace {

constexpr float kMenuSeconds = 8.0f;
constexpr float kEndSeconds = 7.0f;
constexpr float kCaptionSeconds = 5.0f;
constexpr float kCaptionFadeSeconds = 0.6f;

constexpr float kCruiseClosingSpeed = 16.0f;
constexpr float kCruiseUntilDistance = 130.0f;
constexpr float kApproachSpeedPerMetre = 0.12f;
constexpr float kFinalClosingSpeed = 0.45f;
constexpr float kSlowZoneDistance = 30.0f;
constexpr float kSlowZoneClosingSpeed = 1.6f;
constexpr float kStopDistance = 0.6f;
constexpr float kAxialGain = 1.0f;
constexpr float kLateralGain = 0.08f;
constexpr float kLateralDamping = 0.6f;
constexpr float kMaxLateralAccel = 2.5f;
constexpr float kMaxTiltDegrees = 25.0f;
constexpr float kFineTiltDegrees = 6.0f;
constexpr float kFineTiltDistance = 70.0f;
constexpr float kThrustThreshold = 0.4f;
constexpr float kPrecisionThrustThreshold = 0.12f;
constexpr float kBrakeThreshold = 4.0f;
constexpr float kPrecisionDistance = 35.0f;
constexpr float kPodMouseRadiansPerPixel = 0.0020f;
constexpr float kPodSteerGain = 0.1f;
constexpr float kPodMaxSteerRadians = 0.012f;
constexpr float kRollGain = 1.2f;
constexpr float kMaxRollRate = 0.5f;
constexpr float kRollRateDeadband = 0.04f;
constexpr float kDodgeHullFraction = 0.8f;
constexpr float kWeaveUntilDistance = 190.0f;
constexpr float kWeaveMetres = 6.0f;
constexpr float kCockpitOnSeconds = 3.0f;
constexpr float kCockpitOffSeconds = 7.5f;

constexpr float kRevealFromDistance = 120.0f;
constexpr float kRevealUntilDistance = 40.0f;
constexpr float kInterceptFromDistance = 175.0f;
constexpr float kInterceptMaxLateral = 14.0f;
constexpr float kCloseUpFromDistance = 10.0f;
const glm::vec3 kRevealEye(24.0f, 10.0f, 115.0f);
const glm::vec3 kRevealFocus(0.0f, 0.0f, 12.0f);
const glm::vec3 kCloseUpEye(6.5f, 3.0f, 31.5f);
const glm::vec3 kCloseUpTarget(0.0f, 0.0f, 25.5f);

constexpr float kEvaMouseRadiansPerPixel = 0.0022f;
constexpr float kEvaAimGain = 0.08f;
constexpr float kEvaMaxMousePixels = 8.0f;
constexpr float kEvaWalkDelaySeconds = 4.6f;
constexpr float kEvaStopDistance = 2.35f;

float signedAngleAbout(const glm::vec3& from, const glm::vec3& to, const glm::vec3& axis)
{
    return std::atan2(glm::dot(glm::cross(from, to), axis), glm::dot(from, to));
}

glm::vec2 aimAngles(const glm::vec3& direction, const glm::vec3& right, const glm::vec3& up, const glm::vec3& forward)
{
    const float x = glm::dot(direction, right);
    const float y = glm::dot(direction, up);
    const float z = glm::dot(direction, forward);
    return {std::atan2(x, z), std::atan2(y, z)};
}

bool inWindow(float seconds, float start, float end)
{
    return seconds >= start && seconds < end;
}

}

DemoCommands DemoDirector::update(const DemoSnapshot& snapshot, float deltaSeconds)
{
    trackState(snapshot, deltaSeconds);
    queueCaptions(snapshot);
    chooseShot(snapshot);

    DemoCommands commands;
    switch (snapshot.state) {
    case GameState::Menu:
        commands.startMission = stateSeconds_ >= kMenuSeconds;
        break;
    case GameState::Flight:
    case GameState::Docking:
        flyPod(snapshot, commands);
        break;
    case GameState::Eva:
        walkEva(snapshot, commands);
        break;
    case GameState::Win:
    case GameState::Lose:
        commands.quit = stateSeconds_ >= kEndSeconds;
        break;
    }
    commands.hideHud = shot_ != Shot::None;
    fillCaption(commands);
    lastHullFraction_ = snapshot.hullFraction;
    return commands;
}

void DemoDirector::trackState(const DemoSnapshot& snapshot, float deltaSeconds)
{
    totalSeconds_ += deltaSeconds;
    if (snapshot.state != lastState_) {
        lastState_ = snapshot.state;
        stateSeconds_ = 0.0f;
        return;
    }
    stateSeconds_ += deltaSeconds;
}

void DemoDirector::queueCaptions(const DemoSnapshot& snapshot)
{
    const bool justEntered = stateSeconds_ == 0.0f;
    switch (snapshot.state) {
    case GameState::Menu:
        if (totalSeconds_ < 0.1f) {
            showCaption("ORBITAL DOCK", "CGVR mini project: a desktop space docking and repair simulator");
        }
        break;
    case GameState::Flight:
    case GameState::Docking: {
        if (justEntered && snapshot.state == GameState::Flight) {
            showCaption("1. Flight", "Keyboard thrusters with inertia through a drifting debris field");
        }
        const bool cockpitStarted = snapshot.state == GameState::Flight && inWindow(stateSeconds_, kCockpitOnSeconds, kCockpitOnSeconds + 0.02f);
        if (cockpitStarted) {
            showCaption("Cockpit view", "A second camera mode looking out of the pod's window");
        }
        if (!impactCaptioned_ && snapshot.hullFraction < lastHullFraction_) {
            impactCaptioned_ = true;
            showCaption("Debris impact", "Sphere collision, impulse response, hull damage and screen shake");
        }
        if (!revealCaptioned_ && shot_ == Shot::StationReveal) {
            revealCaptioned_ = true;
            showCaption("Rotating station", "Hierarchical transforms: station -> docking arm -> port -> clamps");
        }
        if (!dockingCaptioned_ && snapshot.state == GameState::Docking && shot_ == Shot::None) {
            dockingCaptioned_ = true;
            showCaption("2. Docking", "Match distance, speed, axis and roll with the spinning port, then hold");
        }
        if (!dockedCaptioned_ && snapshot.docked) {
            dockedCaptioned_ = true;
            showCaption("Hard dock", "The pod is re-parented under the port node and the clamps close");
        }
        break;
    }
    case GameState::Eva:
        if (justEntered) {
            showCaption("3. Spacewalk", "First-person EVA on the rotating hull under floodlights and a warning beacon");
        }
        if (!repairCaptioned_ && snapshot.repairing) {
            repairCaptioned_ = true;
            showCaption("Repair", "Hold E for three seconds before life support runs out");
        }
        break;
    case GameState::Win:
        if (justEntered) {
            showCaption("Mission complete", "Blender models, Blinn-Phong lighting, shadow maps, HDR bloom and miniaudio sound");
        }
        break;
    case GameState::Lose:
        break;
    }
}

void DemoDirector::showCaption(const std::string& title, const std::string& text)
{
    captionTitle_ = title;
    captionText_ = text;
    captionStartSeconds_ = totalSeconds_;
}

void DemoDirector::fillCaption(DemoCommands& commands) const
{
    const float age = totalSeconds_ - captionStartSeconds_;
    if (age < 0.0f || age > kCaptionSeconds) {
        return;
    }
    commands.captionTitle = captionTitle_;
    commands.captionText = captionText_;
    const float fadeIn = std::min(age / kCaptionFadeSeconds, 1.0f);
    const float fadeOut = std::min((kCaptionSeconds - age) / kCaptionFadeSeconds, 1.0f);
    commands.captionAlpha = std::min(fadeIn, fadeOut);
}

float DemoDirector::axialDistance(const DemoSnapshot& snapshot) const
{
    return glm::dot(snapshot.podPosition - snapshot.dockingTarget, snapshot.dockingAxis);
}

void DemoDirector::chooseShot(const DemoSnapshot& snapshot)
{
    shot_ = Shot::None;
    const bool flying = snapshot.state == GameState::Flight || snapshot.state == GameState::Docking;
    if (!flying || snapshot.cockpitView) {
        return;
    }
    const float distance = axialDistance(snapshot);
    if (snapshot.docked || distance < kCloseUpFromDistance) {
        shot_ = Shot::DockingCloseUp;
    } else if (distance < kRevealFromDistance && distance > kRevealUntilDistance) {
        shot_ = Shot::StationReveal;
    }
}

std::optional<DemoShot> DemoDirector::cameraShot(const glm::vec3& podPosition) const
{
    switch (shot_) {
    case Shot::StationReveal:
        return DemoShot{kRevealEye, kRevealFocus};
    case Shot::DockingCloseUp:
        return DemoShot{kCloseUpEye, kCloseUpTarget};
    case Shot::None:
        break;
    }
    return std::nullopt;
}

void DemoDirector::flyPod(const DemoSnapshot& snapshot, DemoCommands& commands) const
{
    if (snapshot.docked) {
        return;
    }
    const glm::vec3& axis = snapshot.dockingAxis;
    const float distance = axialDistance(snapshot);
    const float closingSpeed = -glm::dot(snapshot.podVelocity, axis);

    float desiredClosing = distance > kCruiseUntilDistance ? kCruiseClosingSpeed
                                                           : std::clamp(distance * kApproachSpeedPerMetre, kFinalClosingSpeed, kCruiseClosingSpeed);
    if (distance < kSlowZoneDistance) {
        desiredClosing = std::min(desiredClosing, kSlowZoneClosingSpeed);
    }
    if (distance < kStopDistance) {
        desiredClosing = 0.0f;
    }
    glm::vec3 lateral = (snapshot.podPosition - snapshot.dockingTarget) - axis * distance;
    if (distance > kWeaveUntilDistance) {
        lateral -= glm::vec3(1.0f, 0.0f, 0.0f) * std::sin(totalSeconds_ * 0.45f) * kWeaveMetres;
    }
    const bool seekImpact = snapshot.state == GameState::Flight && !impactCaptioned_ && distance > kInterceptFromDistance && snapshot.debrisAhead;
    if (seekImpact) {
        const glm::vec3 debrisOffset = snapshot.debrisAheadPosition - snapshot.dockingTarget;
        glm::vec3 debrisLateral = debrisOffset - axis * glm::dot(debrisOffset, axis);
        if (glm::length(debrisLateral) > kInterceptMaxLateral) {
            debrisLateral = glm::normalize(debrisLateral) * kInterceptMaxLateral;
        }
        lateral -= debrisLateral;
    }
    const glm::vec3 lateralVelocity = snapshot.podVelocity - axis * glm::dot(snapshot.podVelocity, axis);
    glm::vec3 lateralAccel = -lateral * kLateralGain - lateralVelocity * kLateralDamping;
    if (glm::length(lateralAccel) > kMaxLateralAccel) {
        lateralAccel = glm::normalize(lateralAccel) * kMaxLateralAccel;
    }
    const float axialAccel = (desiredClosing - closingSpeed) * kAxialGain;
    const glm::vec3 wanted = -axis * axialAccel + lateralAccel;
    const bool precision = distance < kPrecisionDistance && std::abs(desiredClosing - closingSpeed) < 1.0f;
    if (precision) {
        commands.keysDown.push_back(GLFW_KEY_LEFT_SHIFT);
    }

    glm::vec3 nose = axialAccel < 0.0f ? -wanted : wanted;
    nose = glm::length(nose) > 1e-3f ? glm::normalize(nose) : -axis;
    const float maxTilt = glm::radians(distance < kFineTiltDistance ? kFineTiltDegrees : kMaxTiltDegrees);
    const glm::vec3 perpendicular = nose - axis * glm::dot(nose, axis);
    if (glm::dot(nose, -axis) < std::cos(maxTilt) && glm::length(perpendicular) > 1e-4f) {
        nose = -axis * std::cos(maxTilt) + glm::normalize(perpendicular) * std::sin(maxTilt);
    }
    const glm::vec2 aim = aimAngles(nose, snapshot.podRight, snapshot.podUp, snapshot.podForward);
    const float yawStep = std::clamp(aim.x * kPodSteerGain, -kPodMaxSteerRadians, kPodMaxSteerRadians);
    const float pitchStep = std::clamp(aim.y * kPodSteerGain, -kPodMaxSteerRadians, kPodMaxSteerRadians);
    const float radiansPerPixel = kPodMouseRadiansPerPixel * (precision ? 0.5f : 1.0f);
    commands.mouseDelta = glm::vec2(yawStep, -pitchStep) / radiansPerPixel;

    const float threshold = precision ? kPrecisionThrustThreshold : kThrustThreshold;
    const float alongNose = glm::dot(wanted, snapshot.podForward);
    if (alongNose > threshold) {
        commands.keysDown.push_back(GLFW_KEY_W);
    } else if (alongNose < -kBrakeThreshold && distance > kPrecisionDistance) {
        commands.keysDown.push_back(GLFW_KEY_SPACE);
    } else if (alongNose < -threshold) {
        commands.keysDown.push_back(GLFW_KEY_S);
    }
    const bool dodging = snapshot.state == GameState::Flight && snapshot.closingDebris > 0 && snapshot.hullFraction < kDodgeHullFraction;
    const float alongUp = glm::dot(wanted, snapshot.podUp);
    if (dodging || alongUp > threshold) {
        commands.keysDown.push_back(GLFW_KEY_R);
    } else if (alongUp < -threshold) {
        commands.keysDown.push_back(GLFW_KEY_F);
    }

    const glm::vec3 rollAxis = -snapshot.podForward;
    const float rollError = signedAngleAbout(snapshot.podUp, snapshot.dockingUp, rollAxis);
    const float desiredRollRate = std::clamp(rollError * kRollGain, -kMaxRollRate, kMaxRollRate)
        + snapshot.stationSpinRadiansPerSecond * glm::dot(axis, rollAxis);
    const float rollRate = glm::dot(snapshot.podAngularVelocity, rollAxis);
    if (rollRate < desiredRollRate - kRollRateDeadband) {
        commands.keysDown.push_back(GLFW_KEY_Q);
    } else if (rollRate > desiredRollRate + kRollRateDeadband) {
        commands.keysDown.push_back(GLFW_KEY_E);
    }

    const bool toggleCockpit = snapshot.state == GameState::Flight
        && (inWindow(stateSeconds_, kCockpitOnSeconds, kCockpitOnSeconds + 0.02f) || inWindow(stateSeconds_, kCockpitOffSeconds, kCockpitOffSeconds + 0.02f));
    if (toggleCockpit) {
        commands.keysDown.push_back(GLFW_KEY_V);
    }
}

void DemoDirector::walkEva(const DemoSnapshot& snapshot, DemoCommands& commands) const
{
    if (snapshot.repaired) {
        if (stateSeconds_ < 60.0f) {
            commands.mouseDelta = glm::vec2(0.0f, -1.2f);
        }
        return;
    }
    if (inWindow(stateSeconds_, 1.0f, 2.4f)) {
        commands.mouseDelta = glm::vec2(-6.0f, -1.0f);
        return;
    }
    if (inWindow(stateSeconds_, 2.4f, 4.4f)) {
        commands.mouseDelta = glm::vec2(7.0f, 0.5f);
        return;
    }
    if (stateSeconds_ < 1.0f) {
        return;
    }

    const glm::vec3 toPanel = snapshot.panelPosition - snapshot.cameraPosition;
    const float distance = glm::length(toPanel);
    const glm::vec2 aim = aimAngles(toPanel / std::max(distance, 1e-3f), snapshot.cameraRight, snapshot.cameraUp, snapshot.cameraForward);
    const glm::vec2 pixels(aim.x * kEvaAimGain / kEvaMouseRadiansPerPixel, -aim.y * kEvaAimGain / kEvaMouseRadiansPerPixel);
    commands.mouseDelta = glm::clamp(pixels, glm::vec2(-kEvaMaxMousePixels), glm::vec2(kEvaMaxMousePixels));

    if (stateSeconds_ >= kEvaWalkDelaySeconds && distance > kEvaStopDistance) {
        commands.keysDown.push_back(GLFW_KEY_W);
    }
    if (snapshot.canInteract && distance <= kEvaStopDistance + 0.2f) {
        commands.keysDown.push_back(GLFW_KEY_E);
    }
}

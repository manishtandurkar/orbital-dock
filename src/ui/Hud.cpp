#include "ui/Hud.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

constexpr ImGuiWindowFlags kOverlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
    | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs;

const ImVec4 kGood(0.35f, 1.0f, 0.5f, 1.0f);
const ImVec4 kBad(1.0f, 0.35f, 0.3f, 1.0f);
const ImVec4 kAccent(0.45f, 0.85f, 1.0f, 1.0f);
const ImVec4 kWarning(1.0f, 0.65f, 0.2f, 1.0f);

void beginOverlay(const char* name, const ImVec2& position, const ImVec2& pivot)
{
    ImGui::SetNextWindowPos(position, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::Begin(name, nullptr, kOverlayFlags);
}

void coloredBar(float fraction, const ImVec4& color, const char* label)
{
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::ProgressBar(std::clamp(fraction, 0.0f, 1.0f), ImVec2(220.0f, 18.0f), label);
    ImGui::PopStyleColor();
}

ImVec4 hullColor(float fraction)
{
    if (fraction > 0.6f) {
        return kGood;
    }
    if (fraction > 0.3f) {
        return kWarning;
    }
    return kBad;
}

void toleranceRow(const char* name, float value, float limit, const char* unit, bool ok)
{
    ImGui::TextColored(ok ? kGood : kBad, "%s %-10s %6.2f %s  (< %.1f)", ok ? "[OK]" : "[--]", name, value, unit, limit);
}

ImU32 toColor(const ImVec4& color, float alpha)
{
    return ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, alpha));
}

}

void Hud::draw(const HudData& data) const
{
    drawDamageFlash(data);
    switch (data.state) {
    case GameState::Flight:
        drawFlightPanel(data);
        if (!data.paused) {
            drawNavigation(data);
        }
        break;
    case GameState::Docking:
        drawFlightPanel(data);
        drawDockingPanel(data);
        if (data.alignmentAssist && !data.docked && !data.stereo && !data.paused) {
            drawDockingMarkers(data);
        }
        if (!data.alignmentAssist && !data.docked && !data.paused) {
            drawNavigation(data);
        }
        break;
    case GameState::Eva:
        drawEvaPanel(data);
        break;
    default:
        break;
    }
    drawBanner(data);
    if (data.showDebug) {
        drawDebug(data);
    }
}

void Hud::drawFlightPanel(const HudData& data) const
{
    beginOverlay("FlightPanel", ImVec2(16.0f, 16.0f), ImVec2(0.0f, 0.0f));
    ImGui::TextColored(kAccent, "POD STATUS");
    char hullLabel[32];
    std::snprintf(hullLabel, sizeof(hullLabel), "HULL %.0f%%", data.hullFraction * 100.0f);
    coloredBar(data.hullFraction, hullColor(data.hullFraction), hullLabel);
    ImGui::Text("Speed        %6.1f m/s", data.speed);
    ImGui::Text("Port range   %6.0f m", data.distanceToPort);
    ImGui::Text("View         %s", data.cockpitView ? "cockpit" : "chase");
    if (data.precisionThrust) {
        ImGui::TextColored(kAccent, "FINE THRUST (Shift)");
    }
    if (data.closingDebris > 0) {
        ImGui::TextColored(kBad, "PROXIMITY: %d object%s closing", data.closingDebris, data.closingDebris == 1 ? "" : "s");
    }
    if (data.hullFraction < 0.3f) {
        const bool blink = std::fmod(ImGui::GetTime(), 0.8) < 0.4;
        ImGui::TextColored(blink ? kBad : kWarning, "HULL CRITICAL");
    }
    if (data.leavingMissionArea) {
        const bool blink = std::fmod(ImGui::GetTime(), 0.8) < 0.4;
        ImGui::TextColored(blink ? kBad : kWarning, "LEAVING MISSION AREA: turn back");
    }
    ImGui::End();
}

void Hud::drawDockingPanel(const HudData& data) const
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    beginOverlay("DockingPanel", ImVec2(display.x - 16.0f, 16.0f), ImVec2(1.0f, 0.0f));
    if (data.docked) {
        ImGui::TextColored(kGood, "DOCKING CLAMPS ENGAGED");
        ImGui::Text("Pressurising airlock for EVA...");
        ImGui::End();
        return;
    }
    ImGui::TextColored(kAccent, "DOCKING COMPUTER  [Tab] assist %s", data.alignmentAssist ? "ON" : "OFF");
    const DockingStatus& status = data.docking;
    const DockingTolerance& tolerance = data.tolerance;
    toleranceRow("distance", status.distance, tolerance.maxDistance, "m  ", status.distanceOk);
    toleranceRow("rel speed", status.relativeSpeed, tolerance.maxRelativeSpeed, "m/s", status.speedOk);
    toleranceRow("alignment", status.alignmentDegrees, tolerance.maxAlignmentDegrees, "deg", status.alignmentOk);
    toleranceRow("roll", status.rollDegrees, tolerance.maxRollDegrees, "deg", status.rollOk);
    ImGui::Text("Lateral offset  x %+5.1f  y %+5.1f m", status.lateralOffset.x, status.lateralOffset.y);
    ImGui::Text("Closing speed   %+5.2f m/s", status.closingSpeed);
    coloredBar(status.holdProgress, status.allWithinTolerance() ? kGood : kWarning, "HOLD");
    if (status.distance < 25.0f && status.closingSpeed > 2.0f) {
        const bool blink = std::fmod(ImGui::GetTime(), 0.5) < 0.25;
        ImGui::TextColored(blink ? kBad : kWarning, "TOO FAST: brake (Space) or fine thrust (Shift)");
    }
    if (status.axialDistance < -0.5f) {
        ImGui::TextColored(kBad, "Behind the port: back off and approach along the axis");
    }
    ImGui::End();
}

void Hud::drawDockingMarkers(const HudData& data) const
{
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const bool aligned = data.docking.alignmentOk && data.docking.rollOk;
    if (data.portMarker.visible) {
        const ImVec2 centre(data.portMarker.position.x, data.portMarker.position.y);
        const ImU32 color = toColor(data.docking.distanceOk ? kGood : kAccent, 0.9f);
        drawList->AddCircle(centre, 22.0f, color, 32, 2.0f);
        drawList->AddLine(ImVec2(centre.x - 30.0f, centre.y), ImVec2(centre.x - 14.0f, centre.y), color, 2.0f);
        drawList->AddLine(ImVec2(centre.x + 14.0f, centre.y), ImVec2(centre.x + 30.0f, centre.y), color, 2.0f);
        drawList->AddLine(ImVec2(centre.x, centre.y - 30.0f), ImVec2(centre.x, centre.y - 14.0f), color, 2.0f);
        drawList->AddText(ImVec2(centre.x + 26.0f, centre.y - 30.0f), color, "PORT");
    }
    if (data.boresightMarker.visible) {
        const ImVec2 centre(data.boresightMarker.position.x, data.boresightMarker.position.y);
        const ImU32 color = toColor(aligned ? kGood : kWarning, 0.9f);
        drawList->AddRect(ImVec2(centre.x - 8.0f, centre.y - 8.0f), ImVec2(centre.x + 8.0f, centre.y + 8.0f), color, 0.0f, 0, 2.0f);
        drawList->AddCircleFilled(centre, 2.5f, color);
    }
    if (data.portMarker.visible && data.boresightMarker.visible) {
        drawList->AddLine(ImVec2(data.portMarker.position.x, data.portMarker.position.y),
            ImVec2(data.boresightMarker.position.x, data.boresightMarker.position.y), toColor(kAccent, 0.35f), 1.0f);
    }
}

void Hud::drawNavigation(const HudData& data) const
{
    if (data.stereo) {
        return;
    }
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImU32 color = toColor(kAccent, 0.9f);
    char label[48];
    std::snprintf(label, sizeof(label), "STATION %.0f m", data.distanceToPort);
    if (data.navigationMarker.visible) {
        const ImVec2 centre(data.navigationMarker.position.x, data.navigationMarker.position.y);
        const float size = 12.0f;
        drawList->AddQuad(ImVec2(centre.x, centre.y - size), ImVec2(centre.x + size, centre.y), ImVec2(centre.x, centre.y + size),
            ImVec2(centre.x - size, centre.y), color, 2.0f);
        drawList->AddText(ImVec2(centre.x + 16.0f, centre.y - 8.0f), color, label);
        return;
    }
    if (!data.navigationArrowVisible) {
        return;
    }
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 centre(display.x * 0.5f, display.y * 0.5f);
    const float radius = std::min(display.x, display.y) * 0.4f;
    const glm::vec2 direction = data.navigationArrowDirection;
    const ImVec2 tip(centre.x + direction.x * radius, centre.y + direction.y * radius);
    const glm::vec2 side(-direction.y, direction.x);
    const ImVec2 left(tip.x - direction.x * 22.0f + side.x * 11.0f, tip.y - direction.y * 22.0f + side.y * 11.0f);
    const ImVec2 right(tip.x - direction.x * 22.0f - side.x * 11.0f, tip.y - direction.y * 22.0f - side.y * 11.0f);
    drawList->AddTriangleFilled(tip, left, right, color);
    drawList->AddText(ImVec2(tip.x - direction.x * 60.0f - 40.0f, tip.y - direction.y * 50.0f - 8.0f), color, label);
}

void Hud::drawEvaPanel(const HudData& data) const
{
    beginOverlay("EvaPanel", ImVec2(16.0f, 16.0f), ImVec2(0.0f, 0.0f));
    ImGui::TextColored(kAccent, "EVA SUIT");
    char label[32];
    std::snprintf(label, sizeof(label), "O2 %.0f s", data.lifeSupportSeconds);
    const ImVec4 color = data.lifeSupportFraction > 0.4f ? kGood : (data.lifeSupportFraction > 0.2f ? kWarning : kBad);
    coloredBar(data.lifeSupportFraction, color, label);
    ImGui::Text("Damaged panel  %.1f m", data.distanceToPanel);
    if (data.lifeSupportFraction < 0.2f) {
        const bool blink = std::fmod(ImGui::GetTime(), 0.6) < 0.3;
        ImGui::TextColored(blink ? kBad : kWarning, "OXYGEN LOW");
    }
    ImGui::End();

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 centre(display.x * 0.5f, display.y * 0.5f);
    if (data.stereo) {
        drawList->AddCircle(ImVec2(display.x * 0.25f, centre.y), 3.0f, toColor(kAccent, 0.8f), 12, 1.5f);
        drawList->AddCircle(ImVec2(display.x * 0.75f, centre.y), 3.0f, toColor(kAccent, 0.8f), 12, 1.5f);
    } else {
        drawList->AddCircle(centre, 3.0f, toColor(kAccent, 0.8f), 12, 1.5f);
    }

    if (data.canInteract || data.repairing) {
        beginOverlay("RepairPrompt", ImVec2(centre.x, centre.y + 60.0f), ImVec2(0.5f, 0.0f));
        if (data.repairing) {
            ImGui::TextColored(kWarning, "Repairing panel...");
        } else {
            ImGui::TextColored(kAccent, "Hold [E] to repair the panel");
        }
        coloredBar(data.repairProgress, kGood, "");
        ImGui::End();
    }
}

void Hud::drawDamageFlash(const HudData& data) const
{
    if (data.damageFlash <= 0.01f) {
        return;
    }
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const ImU32 edge = toColor(kBad, std::min(data.damageFlash, 1.0f) * 0.55f);
    const ImU32 clear = toColor(kBad, 0.0f);
    const float band = display.y * 0.18f;
    drawList->AddRectFilledMultiColor(ImVec2(0.0f, 0.0f), ImVec2(display.x, band), edge, edge, clear, clear);
    drawList->AddRectFilledMultiColor(ImVec2(0.0f, display.y - band), ImVec2(display.x, display.y), clear, clear, edge, edge);
    drawList->AddRectFilledMultiColor(ImVec2(0.0f, 0.0f), ImVec2(band, display.y), edge, clear, clear, edge);
    drawList->AddRectFilledMultiColor(ImVec2(display.x - band, 0.0f), ImVec2(display.x, display.y), clear, edge, edge, clear);
}

void Hud::drawBanner(const HudData& data) const
{
    if (data.banner.empty() || data.bannerAlpha <= 0.01f) {
        return;
    }
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.22f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.45f * data.bannerAlpha);
    ImGui::Begin("Banner", nullptr, kOverlayFlags);
    ImGui::SetWindowFontScale(1.6f);
    ImGui::TextColored(ImVec4(kAccent.x, kAccent.y, kAccent.z, data.bannerAlpha), "%s", data.banner.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();
}

void Hud::drawDebug(const HudData& data) const
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    beginOverlay("Debug", ImVec2(16.0f, display.y - 16.0f), ImVec2(0.0f, 1.0f));
    ImGui::TextColored(kWarning, "DEBUG (F1)");
    ImGui::Text("FPS           %.0f", data.framesPerSecond);
    ImGui::Text("State         %s", std::string(gameStateName(data.state)).c_str());
    ImGui::Text("Point lights  %d", data.pointLightCount);
    ImGui::Text("Collision volumes drawn as wireframes");
    ImGui::Text("F2  jump to docking approach");
    ImGui::Text("F4  simulate a debris hit (25%% hull)");
    ImGui::Text("F3  stereo view   F11  fullscreen");
    ImGui::End();
}

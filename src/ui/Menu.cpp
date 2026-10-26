#include "ui/Menu.h"

#include <imgui.h>

namespace {

constexpr float kButtonWidth = 260.0f;
constexpr float kButtonHeight = 38.0f;

void beginCentredWindow(const char* name, float width)
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);
    ImGui::Begin(name, nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoSavedSettings);
}

void centredText(const char* text, const ImVec4& color, float scale)
{
    ImGui::SetWindowFontScale(scale);
    const float width = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - width) * 0.5f);
    ImGui::TextColored(color, "%s", text);
    ImGui::SetWindowFontScale(1.0f);
}

bool centredButton(const char* label)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - kButtonWidth) * 0.5f);
    return ImGui::Button(label, ImVec2(kButtonWidth, kButtonHeight));
}

}

MenuAction Menu::drawMainMenu(bool audioAvailable)
{
    MenuAction action = MenuAction::None;
    beginCentredWindow("MainMenu", 520.0f);
    ImGui::Spacing();
    centredText("ORBITAL DOCK", ImVec4(0.45f, 0.85f, 1.0f, 1.0f), 2.6f);
    centredText("Fly. Dock. Repair. Survive.", ImVec4(1.0f, 0.65f, 0.25f, 1.0f), 1.1f);
    ImGui::Spacing();
    ImGui::TextWrapped(
        "Station Kepler-7 has a failing hull panel and life support is venting. Pilot the maintenance pod through the "
        "debris field, dock with the rotating station, then step out on EVA and repair the panel before your air runs out.");
    ImGui::Spacing();
    if (centredButton("Start Mission")) {
        action = MenuAction::Start;
    }
    if (centredButton(showControls_ ? "Hide Controls" : "Controls")) {
        showControls_ = !showControls_;
    }
    if (centredButton("Quit")) {
        action = MenuAction::Quit;
    }
    if (showControls_) {
        ImGui::Separator();
        drawControls();
    }
    if (!audioAvailable) {
        ImGui::Separator();
        ImGui::TextDisabled("No audio device found: sound is disabled.");
    }
    ImGui::End();
    return action;
}

MenuAction Menu::drawPauseMenu()
{
    MenuAction action = MenuAction::None;
    beginCentredWindow("PauseMenu", 420.0f);
    centredText("PAUSED", ImVec4(0.45f, 0.85f, 1.0f, 1.0f), 2.0f);
    ImGui::Spacing();
    if (centredButton("Resume")) {
        action = MenuAction::Resume;
    }
    if (centredButton("Restart Mission")) {
        action = MenuAction::Restart;
    }
    if (centredButton("Main Menu")) {
        action = MenuAction::MainMenu;
    }
    if (centredButton("Quit")) {
        action = MenuAction::Quit;
    }
    ImGui::Separator();
    drawControls();
    ImGui::End();
    return action;
}

MenuAction Menu::drawEndScreen(const MissionSummary& summary)
{
    MenuAction action = MenuAction::None;
    beginCentredWindow("EndScreen", 480.0f);
    if (summary.won) {
        centredText("MISSION COMPLETE", ImVec4(0.35f, 1.0f, 0.5f, 1.0f), 2.2f);
    } else {
        centredText("MISSION FAILED", ImVec4(1.0f, 0.3f, 0.25f, 1.0f), 2.2f);
    }
    ImGui::Spacing();
    ImGui::TextWrapped("%s", summary.message.c_str());
    ImGui::Separator();
    ImGui::Text("Mission time:    %.1f s", summary.missionSeconds);
    ImGui::Text("Hull integrity:  %.0f %%", summary.hullFraction * 100.0f);
    ImGui::Text("Debris impacts:  %d", summary.impacts);
    if (summary.won) {
        ImGui::Text("Air remaining:   %.1f s", summary.lifeSupportLeftSeconds);
    }
    ImGui::Spacing();
    if (centredButton("Fly Again")) {
        action = MenuAction::Restart;
    }
    if (centredButton("Main Menu")) {
        action = MenuAction::MainMenu;
    }
    if (centredButton("Quit")) {
        action = MenuAction::Quit;
    }
    ImGui::End();
    return action;
}

void Menu::drawControls() const
{
    ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "Flight and docking");
    ImGui::BulletText("W / S  forward / reverse thrust");
    ImGui::BulletText("A / D  yaw,  Q / E  roll,  R / F  up / down");
    ImGui::BulletText("Mouse  pitch and yaw,  Space  brake");
    ImGui::BulletText("Tab  alignment assist,  V  chase / cockpit view");
    ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "EVA");
    ImGui::BulletText("W A S D  move,  Mouse  look,  hold E  repair");
    ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "Global");
    ImGui::BulletText("Esc  pause,  F1  debug overlay");
}

#include "ui/DemoCaption.h"

#include <imgui.h>

namespace {

constexpr ImGuiWindowFlags kCaptionFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs
    | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
    | ImGuiWindowFlags_NoNav;
constexpr float kMarginFraction = 0.04f;
constexpr float kTitleScale = 1.9f;
constexpr float kTextScale = 1.15f;

}

void DemoCaption::draw(const std::string& title, const std::string& text, float alpha) const
{
    if (alpha <= 0.0f || title.empty()) {
        return;
    }
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x * kMarginFraction, display.y * (1.0f - kMarginFraction * 1.6f)), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.55f * alpha);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 12.0f));
    ImGui::Begin("DemoCaption", nullptr, kCaptionFlags);
    ImGui::SetWindowFontScale(kTitleScale);
    ImGui::TextColored(ImVec4(0.45f, 0.85f, 1.0f, 1.0f), "%s", title.c_str());
    ImGui::SetWindowFontScale(kTextScale);
    ImGui::TextUnformatted(text.c_str());
    ImGui::End();
    ImGui::PopStyleVar(2);
}

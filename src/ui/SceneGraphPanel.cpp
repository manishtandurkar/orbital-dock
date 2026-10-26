#include "ui/SceneGraphPanel.h"

#include "scene/SceneNode.h"

#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

namespace {

constexpr size_t kChildrenShownBeforeCollapse = 12;

}

void SceneGraphPanel::draw(const SceneNode& root) const
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x - 16.0f, display.y - 16.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(380.0f, display.y * 0.45f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::Begin("Scene graph (local transforms)", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::TextDisabled("World = parent world x local (T * R * S)");
    drawNode(root);
    ImGui::End();
}

void SceneGraphPanel::drawNode(const SceneNode& node) const
{
    const auto& children = node.children();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (children.size() <= kChildrenShownBeforeCollapse && !children.empty()) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    const bool open = ImGui::TreeNodeEx(static_cast<const void*>(&node), flags, "%s%s", node.name().c_str(),
        node.isVisible() ? "" : " (hidden)");
    if (ImGui::IsItemHovered()) {
        const glm::vec3 position = node.position();
        const glm::vec3 euler = glm::degrees(glm::eulerAngles(node.rotation()));
        const glm::vec3 scale = node.scale();
        ImGui::SetTooltip("T (%.2f, %.2f, %.2f)\nR (%.1f, %.1f, %.1f) deg\nS (%.2f, %.2f, %.2f)", position.x, position.y,
            position.z, euler.x, euler.y, euler.z, scale.x, scale.y, scale.z);
    }
    if (!open) {
        return;
    }
    const glm::vec3 euler = glm::degrees(glm::eulerAngles(node.rotation()));
    ImGui::TextDisabled("pos %.1f %.1f %.1f  rot %.0f %.0f %.0f", node.position().x, node.position().y, node.position().z,
        euler.x, euler.y, euler.z);
    for (const auto& child : children) {
        drawNode(*child);
    }
    ImGui::TreePop();
}

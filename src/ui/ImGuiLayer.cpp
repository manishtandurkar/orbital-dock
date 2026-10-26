#include "ui/ImGuiLayer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

ImGuiLayer::ImGuiLayer(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    applyStyle();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

ImGuiLayer::~ImGuiLayer()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::beginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::endFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::applyStyle()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.03f, 0.05f, 0.08f, 0.82f);
    colors[ImGuiCol_Border] = ImVec4(0.3f, 0.75f, 0.95f, 0.45f);
    colors[ImGuiCol_Button] = ImVec4(0.08f, 0.28f, 0.4f, 0.9f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.45f, 0.62f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.95f, 0.5f, 0.1f, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.3f, 0.85f, 1.0f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.86f, 0.94f, 1.0f, 1.0f);
}

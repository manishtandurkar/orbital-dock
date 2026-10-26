#pragma once

#include <glm/vec2.hpp>

#include <string>

struct GLFWwindow;

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void requestClose();
    void swapBuffers();
    void pollEvents();

    glm::ivec2 framebufferSize() const;
    float aspectRatio() const;
    bool isMinimized() const;

    void toggleFullscreen();
    bool isFullscreen() const;
    void setCursorCaptured(bool captured);
    bool isCursorCaptured() const;

    GLFWwindow* handle() const;

private:
    GLFWwindow* window_ = nullptr;
    bool cursorCaptured_ = false;
    bool fullscreen_ = false;
    glm::ivec2 windowedPosition_{100, 100};
    glm::ivec2 windowedSize_{1280, 720};
};

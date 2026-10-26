#include "core/Window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

namespace {

void reportGlfwError(int code, const char* description)
{
    std::cerr << "GLFW error " << code << ": " << description << '\n';
}

}

Window::Window(int width, int height, const std::string& title)
{
    glfwSetErrorCallback(reportGlfwError);
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialise GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (window_ == nullptr) {
        glfwTerminate();
        throw std::runtime_error("Failed to create an OpenGL 3.3 core window");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        throw std::runtime_error("Failed to load OpenGL functions");
    }

    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << " on " << glGetString(GL_RENDERER) << '\n';
}

Window::~Window()
{
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(window_) != 0;
}

void Window::requestClose()
{
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void Window::swapBuffers()
{
    glfwSwapBuffers(window_);
}

void Window::pollEvents()
{
    glfwPollEvents();
}

glm::ivec2 Window::framebufferSize() const
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    return {width, height};
}

float Window::aspectRatio() const
{
    const glm::ivec2 size = framebufferSize();
    if (size.y <= 0) {
        return 1.0f;
    }
    return static_cast<float>(size.x) / static_cast<float>(size.y);
}

bool Window::isMinimized() const
{
    const glm::ivec2 size = framebufferSize();
    return size.x <= 0 || size.y <= 0;
}

void Window::setCursorCaptured(bool captured)
{
    if (captured == cursorCaptured_) {
        return;
    }
    cursorCaptured_ = captured;
    glfwSetInputMode(window_, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

bool Window::isCursorCaptured() const
{
    return cursorCaptured_;
}

GLFWwindow* Window::handle() const
{
    return window_;
}

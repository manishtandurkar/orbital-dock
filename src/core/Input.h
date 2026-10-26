#pragma once

#include <glm/vec2.hpp>

#include <array>

struct GLFWwindow;

class Input {
public:
    explicit Input(GLFWwindow* window);
    ~Input();

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    void update();
    void resetMouseDelta();

    bool isKeyDown(int key) const;
    bool wasKeyPressed(int key) const;
    bool wasKeyReleased(int key) const;
    bool isMouseButtonDown(int button) const;
    bool wasMouseButtonPressed(int button) const;

    glm::vec2 mouseDelta() const;
    glm::vec2 mousePosition() const;

private:
    static void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);

    static constexpr int kKeyCount = 349;
    static constexpr int kMouseButtonCount = 8;

    GLFWwindow* window_;
    std::array<bool, kKeyCount> currentKeys_{};
    std::array<bool, kKeyCount> previousKeys_{};
    std::array<bool, kKeyCount> pressLatch_{};
    std::array<bool, kKeyCount> pressedThisFrame_{};
    std::array<bool, kMouseButtonCount> currentButtons_{};
    std::array<bool, kMouseButtonCount> previousButtons_{};
    glm::vec2 mousePosition_{0.0f};
    glm::vec2 mouseDelta_{0.0f};
    bool skipNextMouseDelta_ = true;
};

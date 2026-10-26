#include "core/Input.h"

#include <GLFW/glfw3.h>

namespace {

constexpr int kFirstValidKey = GLFW_KEY_SPACE;

bool isGlfwKeyCode(int key)
{
    const bool printable = key == GLFW_KEY_SPACE || key == GLFW_KEY_APOSTROPHE
        || (key >= GLFW_KEY_COMMA && key <= GLFW_KEY_9) || key == GLFW_KEY_SEMICOLON
        || key == GLFW_KEY_EQUAL || (key >= GLFW_KEY_A && key <= GLFW_KEY_RIGHT_BRACKET)
        || key == GLFW_KEY_GRAVE_ACCENT || key == GLFW_KEY_WORLD_1 || key == GLFW_KEY_WORLD_2;
    const bool function = (key >= GLFW_KEY_ESCAPE && key <= GLFW_KEY_END)
        || (key >= GLFW_KEY_CAPS_LOCK && key <= GLFW_KEY_PAUSE)
        || (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25)
        || (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_EQUAL)
        || (key >= GLFW_KEY_LEFT_SHIFT && key <= GLFW_KEY_MENU);
    return printable || function;
}

bool isValidKey(int key, int keyCount)
{
    return key >= 0 && key < keyCount;
}

}

Input::Input(GLFWwindow* window)
    : window_(window)
{
    static_assert(GLFW_KEY_LAST + 1 <= kKeyCount, "Key table too small");
    static_assert(GLFW_MOUSE_BUTTON_LAST + 1 <= kMouseButtonCount, "Mouse table too small");
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, onKeyEvent);
}

Input::~Input()
{
    glfwSetKeyCallback(window_, nullptr);
    glfwSetWindowUserPointer(window_, nullptr);
}

void Input::onKeyEvent(GLFWwindow* window, int key, int, int action, int)
{
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (input != nullptr && action == GLFW_PRESS && isValidKey(key, kKeyCount)) {
        input->pressLatch_[key] = true;
    }
}

void Input::enableScripting()
{
    scripted_ = true;
}

void Input::setScriptedFrame(const std::vector<int>& keysDown, const glm::vec2& mouseDelta)
{
    scriptedKeys_.fill(false);
    for (const int key : keysDown) {
        if (isValidKey(key, kKeyCount)) {
            scriptedKeys_[key] = true;
        }
    }
    scriptedMouseDelta_ = mouseDelta;
}

void Input::update()
{
    previousKeys_ = currentKeys_;
    previousButtons_ = currentButtons_;
    pressedThisFrame_ = pressLatch_;
    pressLatch_.fill(false);

    if (scripted_) {
        pressedThisFrame_.fill(false);
        currentKeys_ = scriptedKeys_;
        currentButtons_.fill(false);
        mouseDelta_ = skipNextMouseDelta_ ? glm::vec2(0.0f) : scriptedMouseDelta_;
        skipNextMouseDelta_ = false;
        return;
    }

    for (int key = kFirstValidKey; key <= GLFW_KEY_LAST; ++key) {
        if (isGlfwKeyCode(key)) {
            currentKeys_[key] = glfwGetKey(window_, key) == GLFW_PRESS;
        }
    }
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        currentButtons_[button] = glfwGetMouseButton(window_, button) == GLFW_PRESS;
    }

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window_, &x, &y);
    const glm::vec2 position(static_cast<float>(x), static_cast<float>(y));
    if (skipNextMouseDelta_) {
        mouseDelta_ = glm::vec2(0.0f);
        skipNextMouseDelta_ = false;
    } else {
        mouseDelta_ = position - mousePosition_;
    }
    mousePosition_ = position;
}

void Input::resetMouseDelta()
{
    skipNextMouseDelta_ = true;
    mouseDelta_ = glm::vec2(0.0f);
}

bool Input::isKeyDown(int key) const
{
    return isValidKey(key, kKeyCount) && currentKeys_[key];
}

bool Input::wasKeyPressed(int key) const
{
    return isValidKey(key, kKeyCount) && (pressedThisFrame_[key] || (currentKeys_[key] && !previousKeys_[key]));
}

bool Input::wasKeyReleased(int key) const
{
    return isValidKey(key, kKeyCount) && !currentKeys_[key] && previousKeys_[key];
}

bool Input::isMouseButtonDown(int button) const
{
    return isValidKey(button, kMouseButtonCount) && currentButtons_[button];
}

bool Input::wasMouseButtonPressed(int button) const
{
    return isValidKey(button, kMouseButtonCount) && currentButtons_[button] && !previousButtons_[button];
}

glm::vec2 Input::mouseDelta() const
{
    return mouseDelta_;
}

glm::vec2 Input::mousePosition() const
{
    return mousePosition_;
}

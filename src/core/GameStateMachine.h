#pragma once

#include <functional>
#include <string_view>

enum class GameState {
    Menu,
    Flight,
    Docking,
    Eva,
    Win,
    Lose
};

std::string_view gameStateName(GameState state);

class GameStateMachine {
public:
    using TransitionHandler = std::function<void(GameState from, GameState to)>;

    explicit GameStateMachine(GameState initial);

    GameState current() const;
    float secondsInState() const;
    bool isPaused() const;

    void transitionTo(GameState next);
    void setPaused(bool paused);
    void advance(float deltaSeconds);
    void setTransitionHandler(TransitionHandler handler);

private:
    GameState current_;
    float secondsInState_ = 0.0f;
    bool paused_ = false;
    TransitionHandler handler_;
};

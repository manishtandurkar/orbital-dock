#include "core/GameStateMachine.h"

#include <utility>

std::string_view gameStateName(GameState state)
{
    switch (state) {
    case GameState::Menu:
        return "Menu";
    case GameState::Flight:
        return "Flight";
    case GameState::Docking:
        return "Docking";
    case GameState::Eva:
        return "EVA";
    case GameState::Win:
        return "Win";
    case GameState::Lose:
        return "Lose";
    }
    return "Unknown";
}

GameStateMachine::GameStateMachine(GameState initial)
    : current_(initial)
{
}

GameState GameStateMachine::current() const
{
    return current_;
}

float GameStateMachine::secondsInState() const
{
    return secondsInState_;
}

bool GameStateMachine::isPaused() const
{
    return paused_;
}

void GameStateMachine::transitionTo(GameState next)
{
    const GameState previous = current_;
    current_ = next;
    secondsInState_ = 0.0f;
    paused_ = false;
    if (handler_) {
        handler_(previous, next);
    }
}

void GameStateMachine::setPaused(bool paused)
{
    paused_ = paused;
}

void GameStateMachine::advance(float deltaSeconds)
{
    if (!paused_) {
        secondsInState_ += deltaSeconds;
    }
}

void GameStateMachine::setTransitionHandler(TransitionHandler handler)
{
    handler_ = std::move(handler);
}

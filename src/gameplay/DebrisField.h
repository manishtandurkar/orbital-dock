#pragma once

#include "gameplay/Debris.h"

#include <glm/vec3.hpp>

#include <random>
#include <vector>

class ModelLibrary;
class SceneNode;

class DebrisField {
public:
    DebrisField(SceneNode& worldRoot, ModelLibrary& models);

    void reset();
    void update(float deltaSeconds, const glm::vec3& podPosition, const glm::vec3& podVelocity,
        const glm::vec3& podForward, bool huntersEnabled);

    std::vector<Debris>& pieces();
    const std::vector<Debris>& pieces() const;
    int countClosingOn(const glm::vec3& podPosition, const glm::vec3& podVelocity, float withinDistance) const;

private:
    float randomRange(float low, float high);
    glm::vec3 randomDirection();
    glm::vec3 randomPointInField();
    bool isOutOfBounds(const glm::vec3& position) const;
    void respawnAwayFrom(Debris& debris, const glm::vec3& podPosition);
    void launchHunter(const glm::vec3& podPosition, const glm::vec3& podVelocity, const glm::vec3& podForward);

    SceneNode* fieldRoot_ = nullptr;
    std::vector<Debris> pieces_;
    std::mt19937 random_;
    float hunterCooldown_ = 0.0f;
};

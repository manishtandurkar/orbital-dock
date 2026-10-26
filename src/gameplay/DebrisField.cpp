#include "gameplay/DebrisField.h"

#include "render/ModelLibrary.h"
#include "scene/SceneNode.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vector_relational.hpp>

#include <array>
#include <string>

namespace {

constexpr int kPieceCount = 72;
constexpr unsigned int kRandomSeed = 1337u;
constexpr glm::vec3 kFieldMin{-90.0f, -60.0f, 70.0f};
constexpr glm::vec3 kFieldMax{90.0f, 60.0f, 320.0f};
constexpr float kBoundsMargin = 40.0f;
constexpr float kMinSpawnDistanceFromPod = 60.0f;
constexpr float kHunterIntervalSeconds = 7.0f;
constexpr float kHunterMinDistance = 85.0f;
constexpr float kHunterMaxDistance = 140.0f;

struct DebrisVariant {
    const char* modelName;
    float minScale;
    float maxScale;
};

constexpr std::array<DebrisVariant, 5> kVariants = {{
    {"debris_rock_a", 1.2f, 3.8f},
    {"debris_rock_b", 1.0f, 3.2f},
    {"debris_panel", 1.0f, 2.2f},
    {"debris_tank", 1.0f, 2.0f},
    {"debris_beam", 1.0f, 2.4f},
}};

}

DebrisField::DebrisField(SceneNode& worldRoot, ModelLibrary& models)
    : random_(kRandomSeed)
{
    fieldRoot_ = &worldRoot.createChild("debris_field");
    pieces_.reserve(kPieceCount);
    for (int i = 0; i < kPieceCount; ++i) {
        const DebrisVariant& variant = kVariants[static_cast<size_t>(i) % kVariants.size()];
        const Model& model = models.get(variant.modelName);
        SceneNode& node = fieldRoot_->createChild("debris_" + std::to_string(i));
        node.setModel(&model);
        pieces_.emplace_back(node, model.boundingRadius());
    }
    reset();
}

void DebrisField::reset()
{
    random_.seed(kRandomSeed);
    hunterCooldown_ = kHunterIntervalSeconds;
    for (size_t i = 0; i < pieces_.size(); ++i) {
        const DebrisVariant& variant = kVariants[i % kVariants.size()];
        const glm::vec3 velocity = randomDirection() * randomRange(0.4f, 2.5f);
        pieces_[i].spawn(randomPointInField(), velocity, randomDirection(), randomRange(0.1f, 0.9f),
            randomRange(variant.minScale, variant.maxScale));
    }
}

void DebrisField::update(float deltaSeconds, const glm::vec3& podPosition, const glm::vec3& podVelocity,
    const glm::vec3& podForward, bool huntersEnabled)
{
    for (Debris& debris : pieces_) {
        debris.update(deltaSeconds);
        if (debris.isActive() && isOutOfBounds(debris.position())) {
            debris.despawn();
        }
        if (!debris.isActive()) {
            respawnAwayFrom(debris, podPosition);
        }
    }

    if (!huntersEnabled) {
        return;
    }
    hunterCooldown_ -= deltaSeconds;
    if (hunterCooldown_ <= 0.0f) {
        hunterCooldown_ = kHunterIntervalSeconds;
        launchHunter(podPosition, podVelocity, podForward);
    }
}

std::vector<Debris>& DebrisField::pieces()
{
    return pieces_;
}

const std::vector<Debris>& DebrisField::pieces() const
{
    return pieces_;
}

int DebrisField::countClosingOn(const glm::vec3& podPosition, const glm::vec3& podVelocity, float withinDistance) const
{
    int count = 0;
    for (const Debris& debris : pieces_) {
        if (!debris.isActive()) {
            continue;
        }
        const glm::vec3 offset = debris.position() - podPosition;
        const float distance = glm::length(offset);
        if (distance > withinDistance || distance < 1e-3f) {
            continue;
        }
        const float closingSpeed = -glm::dot(debris.velocity() - podVelocity, offset / distance);
        if (closingSpeed > 2.0f) {
            ++count;
        }
    }
    return count;
}

float DebrisField::randomRange(float low, float high)
{
    std::uniform_real_distribution<float> distribution(low, high);
    return distribution(random_);
}

glm::vec3 DebrisField::randomDirection()
{
    std::normal_distribution<float> distribution(0.0f, 1.0f);
    glm::vec3 direction(distribution(random_), distribution(random_), distribution(random_));
    if (glm::length(direction) < 1e-4f) {
        return {0.0f, 1.0f, 0.0f};
    }
    return glm::normalize(direction);
}

glm::vec3 DebrisField::randomPointInField()
{
    return {randomRange(kFieldMin.x, kFieldMax.x), randomRange(kFieldMin.y, kFieldMax.y), randomRange(kFieldMin.z, kFieldMax.z)};
}

bool DebrisField::isOutOfBounds(const glm::vec3& position) const
{
    const glm::vec3 low = kFieldMin - glm::vec3(kBoundsMargin);
    const glm::vec3 high = kFieldMax + glm::vec3(kBoundsMargin);
    return glm::any(glm::lessThan(position, low)) || glm::any(glm::greaterThan(position, high));
}

void DebrisField::respawnAwayFrom(Debris& debris, const glm::vec3& podPosition)
{
    glm::vec3 position = randomPointInField();
    for (int attempt = 0; attempt < 8 && glm::distance(position, podPosition) < kMinSpawnDistanceFromPod; ++attempt) {
        position = randomPointInField();
    }
    const size_t index = static_cast<size_t>(&debris - pieces_.data());
    const DebrisVariant& variant = kVariants[index % kVariants.size()];
    debris.spawn(position, randomDirection() * randomRange(0.4f, 2.5f), randomDirection(), randomRange(0.1f, 0.9f),
        randomRange(variant.minScale, variant.maxScale));
}

void DebrisField::launchHunter(const glm::vec3& podPosition, const glm::vec3& podVelocity, const glm::vec3& podForward)
{
    Debris* candidate = nullptr;
    float farthest = 0.0f;
    for (Debris& debris : pieces_) {
        const float distance = glm::distance(debris.position(), podPosition);
        if (debris.isActive() && distance > farthest) {
            farthest = distance;
            candidate = &debris;
        }
    }
    if (candidate == nullptr || farthest < kHunterMinDistance) {
        return;
    }

    const glm::vec3 lateral = randomDirection() * randomRange(5.0f, 25.0f);
    const glm::vec3 start = podPosition + podForward * randomRange(kHunterMinDistance, kHunterMaxDistance) + lateral;
    const float speed = randomRange(4.0f, 7.0f);
    const float travelSeconds = glm::distance(start, podPosition) / (speed + glm::length(podVelocity));
    const glm::vec3 aimPoint = podPosition + podVelocity * travelSeconds;
    const glm::vec3 velocity = glm::normalize(aimPoint - start) * speed;
    const size_t index = static_cast<size_t>(candidate - pieces_.data());
    const DebrisVariant& variant = kVariants[index % kVariants.size()];
    candidate->spawn(start, velocity, randomDirection(), randomRange(0.3f, 1.2f), randomRange(variant.minScale, variant.maxScale));
}

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct DockingTolerance {
    float maxDistance = 1.5f;
    float maxRelativeSpeed = 1.2f;
    float maxAlignmentDegrees = 10.0f;
    float maxRollDegrees = 15.0f;
    float holdSeconds = 1.0f;
};

struct DockingInputs {
    glm::vec3 podPosition{0.0f};
    glm::vec3 podVelocity{0.0f};
    glm::vec3 podForward{0.0f, 0.0f, -1.0f};
    glm::vec3 podUp{0.0f, 1.0f, 0.0f};
    glm::vec3 targetPosition{0.0f};
    glm::vec3 targetVelocity{0.0f};
    glm::vec3 portAxis{0.0f, 0.0f, 1.0f};
    glm::vec3 portUp{0.0f, 1.0f, 0.0f};
};

struct DockingStatus {
    float distance = 0.0f;
    float axialDistance = 0.0f;
    glm::vec2 lateralOffset{0.0f};
    float relativeSpeed = 0.0f;
    float closingSpeed = 0.0f;
    float alignmentDegrees = 0.0f;
    float rollDegrees = 0.0f;
    bool distanceOk = false;
    bool speedOk = false;
    bool alignmentOk = false;
    bool rollOk = false;
    float holdProgress = 0.0f;
    bool locked = false;

    bool allWithinTolerance() const
    {
        return distanceOk && speedOk && alignmentOk && rollOk;
    }
};

class DockingCheck {
public:
    explicit DockingCheck(DockingTolerance tolerance = {});

    DockingStatus evaluate(const DockingInputs& inputs, float deltaSeconds);
    void reset();
    const DockingTolerance& tolerance() const;

private:
    DockingTolerance tolerance_;
    float holdTimer_ = 0.0f;
};

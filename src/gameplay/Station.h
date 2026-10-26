#pragma once

#include "physics/Aabb.h"
#include "physics/Sphere.h"
#include "render/Light.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <vector>

class ModelLibrary;
class SceneNode;

struct StationCollider {
    const SceneNode* node = nullptr;
    Aabb localBox;
    bool softCapture = false;
};

class Station {
public:
    static constexpr float kSpinDegreesPerSecond = 3.0f;

    Station(SceneNode& worldRoot, ModelLibrary& models);

    void reset();
    void update(float deltaSeconds, double elapsedSeconds);

    void setArmDeployed(bool deployed);
    void deployArmImmediately();
    bool isArmDeployed() const;
    void setClampsClosed(bool closed);
    bool areClampsClosed() const;
    void setPanelRepaired(bool repaired);
    void setAlarmActive(bool active);

    SceneNode& root();
    SceneNode& dockingPort();
    SceneNode& deck();
    const SceneNode& repairPanel() const;

    glm::vec3 spinAxisWorld() const;
    float spinRadiansPerSecond() const;
    glm::vec3 pointVelocity(const glm::vec3& worldPoint) const;
    glm::vec3 dockingTargetWorld() const;
    glm::vec3 dockingAxisWorld() const;
    glm::vec3 dockingUpWorld() const;
    glm::vec3 approachTargetWorld() const;
    glm::vec3 approachUpWorld() const;
    glm::mat4 dockedPodLocalTransform() const;

    const std::vector<StationCollider>& colliders() const;
    std::vector<Sphere> worldSphereColliders() const;
    void appendLights(std::vector<PointLight>& lights) const;

    float warningLightLevel() const;

private:
    void buildHierarchy(ModelLibrary& models);
    void buildColliders();
    void updateArm(float deltaSeconds);
    void updateClamps(float deltaSeconds);
    void updateLights(double elapsedSeconds);

    SceneNode* root_ = nullptr;
    SceneNode* hub_ = nullptr;
    SceneNode* ring_ = nullptr;
    SceneNode* solar_ = nullptr;
    SceneNode* arm_ = nullptr;
    SceneNode* port_ = nullptr;
    SceneNode* deck_ = nullptr;
    SceneNode* repairPanelDamaged_ = nullptr;
    SceneNode* repairPanelFixed_ = nullptr;
    SceneNode* warningLight_ = nullptr;
    std::array<SceneNode*, 4> clamps_{};
    std::vector<SceneNode*> floodlights_;

    std::vector<StationCollider> colliders_;
    std::vector<std::pair<const SceneNode*, Sphere>> sphereColliders_;

    float spinAngleDegrees_ = 0.0f;
    float armAngleDegrees_ = 0.0f;
    bool armDeployed_ = false;
    float clampAngleDegrees_ = 0.0f;
    bool clampsClosed_ = false;
    bool panelRepaired_ = false;
    bool alarmActive_ = true;
    float warningLevel_ = 0.0f;
};

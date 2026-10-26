#pragma once

#include "render/Renderer.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class Model;

class SceneNode {
public:
    explicit SceneNode(std::string name);

    SceneNode(const SceneNode&) = delete;
    SceneNode& operator=(const SceneNode&) = delete;

    SceneNode& createChild(std::string name);
    SceneNode& addChild(std::unique_ptr<SceneNode> child);
    std::unique_ptr<SceneNode> detachFromParent();
    void reparentTo(SceneNode& newParent, bool keepWorldTransform);
    void clearChildren();

    const std::string& name() const;
    SceneNode* parent() const;
    const std::vector<std::unique_ptr<SceneNode>>& children() const;

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setScale(const glm::vec3& scale);
    void setLocalTransform(const glm::mat4& transform);

    const glm::vec3& position() const;
    const glm::quat& rotation() const;
    const glm::vec3& scale() const;

    glm::mat4 localMatrix() const;
    glm::mat4 worldMatrix() const;
    glm::vec3 worldPosition() const;
    glm::quat worldRotation() const;
    glm::vec3 localToWorldPoint(const glm::vec3& localPoint) const;
    glm::vec3 localToWorldDirection(const glm::vec3& localDirection) const;

    void setModel(const Model* model);
    const Model* model() const;
    void setVisible(bool visible);
    bool isVisible() const;
    DrawStyle& drawStyle();
    const DrawStyle& drawStyle() const;

    void visit(const std::function<void(const SceneNode&, const glm::mat4&)>& visitor) const;

private:
    void visitWithParent(const std::function<void(const SceneNode&, const glm::mat4&)>& visitor,
        const glm::mat4& parentWorld) const;

    std::string name_;
    SceneNode* parent_ = nullptr;
    std::vector<std::unique_ptr<SceneNode>> children_;
    glm::vec3 position_{0.0f};
    glm::quat rotation_{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale_{1.0f};
    const Model* model_ = nullptr;
    bool visible_ = true;
    DrawStyle drawStyle_;
};

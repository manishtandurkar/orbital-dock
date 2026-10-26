#include "scene/SceneNode.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <algorithm>
#include <utility>

SceneNode::SceneNode(std::string name)
    : name_(std::move(name))
{
}

SceneNode& SceneNode::createChild(std::string name)
{
    return addChild(std::make_unique<SceneNode>(std::move(name)));
}

SceneNode& SceneNode::addChild(std::unique_ptr<SceneNode> child)
{
    child->parent_ = this;
    children_.push_back(std::move(child));
    return *children_.back();
}

std::unique_ptr<SceneNode> SceneNode::detachFromParent()
{
    if (parent_ == nullptr) {
        return nullptr;
    }
    auto& siblings = parent_->children_;
    const auto found = std::find_if(siblings.begin(), siblings.end(),
        [this](const std::unique_ptr<SceneNode>& sibling) { return sibling.get() == this; });
    if (found == siblings.end()) {
        return nullptr;
    }
    std::unique_ptr<SceneNode> self = std::move(*found);
    siblings.erase(found);
    parent_ = nullptr;
    return self;
}

void SceneNode::reparentTo(SceneNode& newParent, bool keepWorldTransform)
{
    if (parent_ == &newParent) {
        return;
    }
    const glm::mat4 world = worldMatrix();
    std::unique_ptr<SceneNode> self = detachFromParent();
    if (!self) {
        return;
    }
    if (keepWorldTransform) {
        setLocalTransform(glm::inverse(newParent.worldMatrix()) * world);
    }
    newParent.addChild(std::move(self));
}

void SceneNode::clearChildren()
{
    children_.clear();
}

const std::string& SceneNode::name() const
{
    return name_;
}

SceneNode* SceneNode::parent() const
{
    return parent_;
}

const std::vector<std::unique_ptr<SceneNode>>& SceneNode::children() const
{
    return children_;
}

void SceneNode::setPosition(const glm::vec3& position)
{
    position_ = position;
}

void SceneNode::setRotation(const glm::quat& rotation)
{
    rotation_ = glm::normalize(rotation);
}

void SceneNode::setScale(const glm::vec3& scale)
{
    scale_ = scale;
}

void SceneNode::setLocalTransform(const glm::mat4& transform)
{
    glm::vec3 scale(1.0f);
    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 translation(0.0f);
    glm::vec3 skew(0.0f);
    glm::vec4 perspective(0.0f);
    if (glm::decompose(transform, scale, rotation, translation, skew, perspective)) {
        position_ = translation;
        rotation_ = glm::normalize(rotation);
        scale_ = scale;
    }
}

const glm::vec3& SceneNode::position() const
{
    return position_;
}

const glm::quat& SceneNode::rotation() const
{
    return rotation_;
}

const glm::vec3& SceneNode::scale() const
{
    return scale_;
}

glm::mat4 SceneNode::localMatrix() const
{
    return glm::translate(glm::mat4(1.0f), position_) * glm::mat4_cast(rotation_) * glm::scale(glm::mat4(1.0f), scale_);
}

glm::mat4 SceneNode::worldMatrix() const
{
    if (parent_ == nullptr) {
        return localMatrix();
    }
    return parent_->worldMatrix() * localMatrix();
}

glm::vec3 SceneNode::worldPosition() const
{
    return glm::vec3(worldMatrix()[3]);
}

glm::quat SceneNode::worldRotation() const
{
    if (parent_ == nullptr) {
        return rotation_;
    }
    return glm::normalize(parent_->worldRotation() * rotation_);
}

glm::vec3 SceneNode::localToWorldPoint(const glm::vec3& localPoint) const
{
    return glm::vec3(worldMatrix() * glm::vec4(localPoint, 1.0f));
}

glm::vec3 SceneNode::localToWorldDirection(const glm::vec3& localDirection) const
{
    return worldRotation() * localDirection;
}

void SceneNode::setModel(const Model* model)
{
    model_ = model;
}

const Model* SceneNode::model() const
{
    return model_;
}

void SceneNode::setVisible(bool visible)
{
    visible_ = visible;
}

bool SceneNode::isVisible() const
{
    return visible_;
}

DrawStyle& SceneNode::drawStyle()
{
    return drawStyle_;
}

const DrawStyle& SceneNode::drawStyle() const
{
    return drawStyle_;
}

void SceneNode::visit(const std::function<void(const SceneNode&, const glm::mat4&)>& visitor) const
{
    const glm::mat4 parentWorld = parent_ == nullptr ? glm::mat4(1.0f) : parent_->worldMatrix();
    visitWithParent(visitor, parentWorld);
}

void SceneNode::visitWithParent(const std::function<void(const SceneNode&, const glm::mat4&)>& visitor,
    const glm::mat4& parentWorld) const
{
    if (!visible_) {
        return;
    }
    const glm::mat4 world = parentWorld * localMatrix();
    visitor(*this, world);
    for (const auto& child : children_) {
        child->visitWithParent(visitor, world);
    }
}

#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <vector>

class DebugLines {
public:
    DebugLines();
    ~DebugLines();

    DebugLines(const DebugLines&) = delete;
    DebugLines& operator=(const DebugLines&) = delete;

    void addLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    void addBox(const glm::mat4& transform, const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec3& color);
    void addSphere(const glm::vec3& center, float radius, const glm::vec3& color);

    bool empty() const;
    void upload();
    void draw() const;
    void clear();

private:
    struct LineVertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    std::vector<LineVertex> vertices_;
    unsigned int vertexArray_ = 0;
    unsigned int vertexBuffer_ = 0;
    int uploadedCount_ = 0;
};

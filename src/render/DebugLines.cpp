#include "render/DebugLines.h"

#include <glad/glad.h>
#include <glm/gtc/constants.hpp>

#include <array>
#include <cmath>
#include <cstddef>

namespace {

constexpr int kCircleSegments = 24;

}

DebugLines::DebugLines()
{
    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), reinterpret_cast<void*>(offsetof(LineVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), reinterpret_cast<void*>(offsetof(LineVertex, color)));
    glBindVertexArray(0);
}

DebugLines::~DebugLines()
{
    glDeleteBuffers(1, &vertexBuffer_);
    glDeleteVertexArrays(1, &vertexArray_);
}

void DebugLines::addLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
{
    vertices_.push_back({from, color});
    vertices_.push_back({to, color});
}

void DebugLines::addBox(const glm::mat4& transform, const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec3& color)
{
    std::array<glm::vec3, 8> corners;
    for (int i = 0; i < 8; ++i) {
        const glm::vec3 local((i & 1) ? maxCorner.x : minCorner.x, (i & 2) ? maxCorner.y : minCorner.y,
            (i & 4) ? maxCorner.z : minCorner.z);
        corners[i] = glm::vec3(transform * glm::vec4(local, 1.0f));
    }
    const std::array<std::array<int, 2>, 12> edges = {{
        {0, 1}, {2, 3}, {4, 5}, {6, 7},
        {0, 2}, {1, 3}, {4, 6}, {5, 7},
        {0, 4}, {1, 5}, {2, 6}, {3, 7},
    }};
    for (const auto& edge : edges) {
        addLine(corners[edge[0]], corners[edge[1]], color);
    }
}

void DebugLines::addSphere(const glm::vec3& center, float radius, const glm::vec3& color)
{
    for (int axis = 0; axis < 3; ++axis) {
        for (int i = 0; i < kCircleSegments; ++i) {
            const float a0 = glm::two_pi<float>() * static_cast<float>(i) / kCircleSegments;
            const float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) / kCircleSegments;
            glm::vec3 p0(0.0f);
            glm::vec3 p1(0.0f);
            p0[(axis + 1) % 3] = std::cos(a0) * radius;
            p0[(axis + 2) % 3] = std::sin(a0) * radius;
            p1[(axis + 1) % 3] = std::cos(a1) * radius;
            p1[(axis + 2) % 3] = std::sin(a1) * radius;
            addLine(center + p0, center + p1, color);
        }
    }
}

bool DebugLines::empty() const
{
    return vertices_.empty();
}

void DebugLines::upload()
{
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(LineVertex)), vertices_.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    uploadedCount_ = static_cast<int>(vertices_.size());
}

void DebugLines::draw() const
{
    if (uploadedCount_ == 0) {
        return;
    }
    glBindVertexArray(vertexArray_);
    glDrawArrays(GL_LINES, 0, uploadedCount_);
    glBindVertexArray(0);
}

void DebugLines::clear()
{
    vertices_.clear();
    uploadedCount_ = 0;
}

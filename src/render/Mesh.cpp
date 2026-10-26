#include "render/Mesh.h"

#include <glad/glad.h>
#include <glm/common.hpp>

#include <array>
#include <cstddef>
#include <limits>
#include <utility>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
    : indexCount_(static_cast<int>(indices.size()))
{
    if (!vertices.empty()) {
        boundsMin_ = glm::vec3(std::numeric_limits<float>::max());
        boundsMax_ = glm::vec3(std::numeric_limits<float>::lowest());
        for (const Vertex& vertex : vertices) {
            boundsMin_ = glm::min(boundsMin_, vertex.position);
            boundsMax_ = glm::max(boundsMax_, vertex.position);
        }
    }

    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &indexBuffer_);

    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));

    glBindVertexArray(0);
}

Mesh::~Mesh()
{
    release();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vertexArray_(std::exchange(other.vertexArray_, 0))
    , vertexBuffer_(std::exchange(other.vertexBuffer_, 0))
    , indexBuffer_(std::exchange(other.indexBuffer_, 0))
    , indexCount_(std::exchange(other.indexCount_, 0))
    , boundsMin_(other.boundsMin_)
    , boundsMax_(other.boundsMax_)
{
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        release();
        vertexArray_ = std::exchange(other.vertexArray_, 0);
        vertexBuffer_ = std::exchange(other.vertexBuffer_, 0);
        indexBuffer_ = std::exchange(other.indexBuffer_, 0);
        indexCount_ = std::exchange(other.indexCount_, 0);
        boundsMin_ = other.boundsMin_;
        boundsMax_ = other.boundsMax_;
    }
    return *this;
}

void Mesh::release()
{
    if (indexBuffer_ != 0) {
        glDeleteBuffers(1, &indexBuffer_);
        indexBuffer_ = 0;
    }
    if (vertexBuffer_ != 0) {
        glDeleteBuffers(1, &vertexBuffer_);
        vertexBuffer_ = 0;
    }
    if (vertexArray_ != 0) {
        glDeleteVertexArrays(1, &vertexArray_);
        vertexArray_ = 0;
    }
}

Mesh Mesh::createUnitCube()
{
    struct Face {
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };
    const std::array<Face, 6> faces = {{
        {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    }};

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    const std::array<glm::vec2, 4> corners = {{{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}}};
    for (const Face& face : faces) {
        const auto base = static_cast<std::uint32_t>(vertices.size());
        for (const glm::vec2& corner : corners) {
            const glm::vec3 position = 0.5f * (face.normal + corner.x * face.tangent + corner.y * face.bitangent);
            vertices.push_back({position, face.normal, corner * 0.5f + 0.5f});
        }
        indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    return Mesh(vertices, indices);
}

void Mesh::draw() const
{
    glBindVertexArray(vertexArray_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

const glm::vec3& Mesh::boundsMin() const
{
    return boundsMin_;
}

const glm::vec3& Mesh::boundsMax() const
{
    return boundsMax_;
}

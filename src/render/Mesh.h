#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    static Mesh createUnitCube();

    void draw() const;

    const glm::vec3& boundsMin() const;
    const glm::vec3& boundsMax() const;

private:
    void release();

    unsigned int vertexArray_ = 0;
    unsigned int vertexBuffer_ = 0;
    unsigned int indexBuffer_ = 0;
    int indexCount_ = 0;
    glm::vec3 boundsMin_{0.0f};
    glm::vec3 boundsMax_{0.0f};
};

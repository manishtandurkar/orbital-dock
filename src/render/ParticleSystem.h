#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <random>
#include <vector>

struct ParticleBurst {
    glm::vec3 position{0.0f};
    glm::vec3 baseVelocity{0.0f};
    float spreadSpeed = 3.0f;
    glm::vec4 color{1.0f, 0.6f, 0.2f, 1.0f};
    float lifeSeconds = 0.6f;
    float size = 0.08f;
    int count = 20;
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    void emit(const ParticleBurst& burst);
    void update(float deltaSeconds);
    void clear();
    void draw();
    int liveCount() const;

private:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 color;
        float size;
        float age;
        float life;
    };

    struct GpuParticle {
        glm::vec3 position;
        glm::vec4 color;
        float size;
    };

    std::vector<Particle> particles_;
    std::vector<GpuParticle> gpuParticles_;
    std::mt19937 random_;
    unsigned int vertexArray_ = 0;
    unsigned int vertexBuffer_ = 0;
};

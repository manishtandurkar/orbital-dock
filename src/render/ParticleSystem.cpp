#include "render/ParticleSystem.h"

#include <glad/glad.h>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cstddef>

namespace {

constexpr size_t kMaxParticles = 4000;

}

ParticleSystem::ParticleSystem()
    : random_(99u)
{
    particles_.reserve(kMaxParticles);
    gpuParticles_.reserve(kMaxParticles);
    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GpuParticle), reinterpret_cast<void*>(offsetof(GpuParticle, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GpuParticle), reinterpret_cast<void*>(offsetof(GpuParticle, color)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(GpuParticle), reinterpret_cast<void*>(offsetof(GpuParticle, size)));
    glBindVertexArray(0);
}

ParticleSystem::~ParticleSystem()
{
    glDeleteBuffers(1, &vertexBuffer_);
    glDeleteVertexArrays(1, &vertexArray_);
}

void ParticleSystem::emit(const ParticleBurst& burst)
{
    std::normal_distribution<float> direction(0.0f, 1.0f);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (int i = 0; i < burst.count && particles_.size() < kMaxParticles; ++i) {
        glm::vec3 random(direction(random_), direction(random_), direction(random_));
        if (glm::length(random) > 1e-4f) {
            random = glm::normalize(random);
        }
        Particle particle;
        particle.position = burst.position;
        particle.velocity = burst.baseVelocity + random * burst.spreadSpeed * (0.3f + 0.7f * unit(random_));
        particle.color = burst.color;
        particle.size = burst.size * (0.6f + 0.8f * unit(random_));
        particle.age = 0.0f;
        particle.life = burst.lifeSeconds * (0.5f + unit(random_));
        particles_.push_back(particle);
    }
}

void ParticleSystem::update(float deltaSeconds)
{
    for (Particle& particle : particles_) {
        particle.age += deltaSeconds;
        particle.position += particle.velocity * deltaSeconds;
        particle.velocity *= 1.0f - std::min(deltaSeconds * 0.8f, 1.0f);
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                         [](const Particle& particle) { return particle.age >= particle.life; }),
        particles_.end());
}

void ParticleSystem::clear()
{
    particles_.clear();
}

void ParticleSystem::draw()
{
    if (particles_.empty()) {
        return;
    }
    gpuParticles_.clear();
    for (const Particle& particle : particles_) {
        const float fade = 1.0f - particle.age / particle.life;
        glm::vec4 color = particle.color;
        color.a *= fade;
        color = glm::vec4(glm::vec3(color) * fade, color.a);
        gpuParticles_.push_back({particle.position, color, particle.size});
    }
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(gpuParticles_.size() * sizeof(GpuParticle)), gpuParticles_.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(gpuParticles_.size()));
    glBindVertexArray(0);
}

int ParticleSystem::liveCount() const
{
    return static_cast<int>(particles_.size());
}

#include "render/ShadowMap.h"

#include <glad/glad.h>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <stdexcept>

ShadowMap::ShadowMap(int resolution)
    : resolution_(resolution)
{
    glGenTextures(1, &depthTexture_);
    glBindTexture(GL_TEXTURE_2D, depthTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture_, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Shadow map framebuffer is incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowMap::~ShadowMap()
{
    glDeleteFramebuffers(1, &framebuffer_);
    glDeleteTextures(1, &depthTexture_);
}

void ShadowMap::focus(const glm::vec3& center, float radius, const glm::vec3& lightDirection)
{
    const glm::vec3 direction = glm::normalize(lightDirection);
    const glm::vec3 up = std::abs(direction.y) > 0.95f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 lightView = glm::lookAt(-direction * radius * 2.0f, glm::vec3(0.0f), up);

    const float texelWorldSize = 2.0f * radius / static_cast<float>(resolution_);
    glm::vec3 centerInLight = glm::vec3(lightView * glm::vec4(center, 1.0f));
    centerInLight.x = std::floor(centerInLight.x / texelWorldSize) * texelWorldSize;
    centerInLight.y = std::floor(centerInLight.y / texelWorldSize) * texelWorldSize;

    const glm::mat4 projection = glm::ortho(centerInLight.x - radius, centerInLight.x + radius, centerInLight.y - radius,
        centerInLight.y + radius, -centerInLight.z - radius * 3.0f, -centerInLight.z + radius * 3.0f);
    lightSpace_ = projection * lightView;
}

void ShadowMap::bindForWriting() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, resolution_, resolution_);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::bindTexture(int unit) const
{
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
    glBindTexture(GL_TEXTURE_2D, depthTexture_);
}

const glm::mat4& ShadowMap::lightSpaceMatrix() const
{
    return lightSpace_;
}

int ShadowMap::resolution() const
{
    return resolution_;
}

#include "render/PostProcess.h"

#include "render/GlDebug.h"

#include <glm/common.hpp>

#include <glad/glad.h>

#include <stdexcept>

namespace {

constexpr int kSamples = 4;
constexpr float kBloomThreshold = 1.0f;
constexpr float kBloomStrength = 0.45f;
constexpr float kExposure = 0.9f;
constexpr float kVignetteStrength = 0.55f;
constexpr int kBlurPasses = 3;

unsigned int createColorTexture(const glm::ivec2& size)
{
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

unsigned int createFramebufferForTexture(unsigned int texture)
{
    unsigned int framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Post-process framebuffer is incomplete");
    }
    return framebuffer;
}

}

PostProcess::PostProcess()
    : extractShader_("shaders/fullscreen.vert", "shaders/bloom_extract.frag")
    , blurShader_("shaders/fullscreen.vert", "shaders/blur.frag")
    , compositeShader_("shaders/fullscreen.vert", "shaders/composite.frag")
{
    glGenVertexArrays(1, &emptyVertexArray_);
}

PostProcess::~PostProcess()
{
    destroyTargets();
    glDeleteVertexArrays(1, &emptyVertexArray_);
}

void PostProcess::createTargets(const glm::ivec2& size)
{
    destroyTargets();
    size_ = size;
    bloomSize_ = glm::max(size / 2, glm::ivec2(1));

    glGenFramebuffers(1, &multisampleFramebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, multisampleFramebuffer_);
    glGenRenderbuffers(1, &multisampleColor_);
    glBindRenderbuffer(GL_RENDERBUFFER, multisampleColor_);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, kSamples, GL_RGBA16F, size.x, size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, multisampleColor_);
    glGenRenderbuffers(1, &multisampleDepth_);
    glBindRenderbuffer(GL_RENDERBUFFER, multisampleDepth_);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, kSamples, GL_DEPTH_COMPONENT24, size.x, size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, multisampleDepth_);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Multisample scene framebuffer is incomplete");
    }

    resolveColor_ = createColorTexture(size);
    resolveFramebuffer_ = createFramebufferForTexture(resolveColor_);
    for (size_t i = 0; i < bloomColors_.size(); ++i) {
        bloomColors_[i] = createColorTexture(bloomSize_);
        bloomFramebuffers_[i] = createFramebufferForTexture(bloomColors_[i]);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gldebug::checkErrors("PostProcess::createTargets");
}

void PostProcess::destroyTargets()
{
    glDeleteFramebuffers(1, &multisampleFramebuffer_);
    glDeleteRenderbuffers(1, &multisampleColor_);
    glDeleteRenderbuffers(1, &multisampleDepth_);
    glDeleteFramebuffers(1, &resolveFramebuffer_);
    glDeleteTextures(1, &resolveColor_);
    glDeleteFramebuffers(static_cast<GLsizei>(bloomFramebuffers_.size()), bloomFramebuffers_.data());
    glDeleteTextures(static_cast<GLsizei>(bloomColors_.size()), bloomColors_.data());
    multisampleFramebuffer_ = 0;
    multisampleColor_ = 0;
    multisampleDepth_ = 0;
    resolveFramebuffer_ = 0;
    resolveColor_ = 0;
    bloomFramebuffers_ = {};
    bloomColors_ = {};
}

void PostProcess::beginScene(const glm::ivec2& framebufferSize)
{
    if (framebufferSize != size_) {
        createTargets(framebufferSize);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, multisampleFramebuffer_);
}

void PostProcess::endScene()
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampleFramebuffer_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFramebuffer_);
    glBlitFramebuffer(0, 0, size_.x, size_.y, 0, 0, size_.x, size_.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(emptyVertexArray_);

    glViewport(0, 0, bloomSize_.x, bloomSize_.y);
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffers_[0]);
    extractShader_.use();
    extractShader_.setInt("uScene", 0);
    extractShader_.setFloat("uThreshold", kBloomThreshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, resolveColor_);
    drawFullscreen();

    blurShader_.use();
    blurShader_.setInt("uImage", 0);
    for (int pass = 0; pass < kBlurPasses * 2; ++pass) {
        const int source = pass % 2;
        const int target = 1 - source;
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffers_[target]);
        blurShader_.setVec2("uDirection", source == 0 ? glm::vec2(1.0f, 0.0f) : glm::vec2(0.0f, 1.0f));
        glBindTexture(GL_TEXTURE_2D, bloomColors_[source]);
        drawFullscreen();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, size_.x, size_.y);
    compositeShader_.use();
    compositeShader_.setInt("uScene", 0);
    compositeShader_.setInt("uBloom", 1);
    compositeShader_.setFloat("uExposure", kExposure);
    compositeShader_.setFloat("uBloomStrength", kBloomStrength);
    compositeShader_.setFloat("uVignetteStrength", kVignetteStrength);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, resolveColor_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomColors_[0]);
    drawFullscreen();
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    gldebug::checkErrors("PostProcess::endScene");
}

void PostProcess::drawFullscreen() const
{
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

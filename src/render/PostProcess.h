#pragma once

#include "render/Shader.h"

#include <glm/vec2.hpp>

#include <array>

class PostProcess {
public:
    PostProcess();
    ~PostProcess();

    PostProcess(const PostProcess&) = delete;
    PostProcess& operator=(const PostProcess&) = delete;

    void beginScene(const glm::ivec2& framebufferSize);
    void endScene();

private:
    void createTargets(const glm::ivec2& size);
    void destroyTargets();
    void drawFullscreen() const;

    Shader extractShader_;
    Shader blurShader_;
    Shader compositeShader_;
    unsigned int emptyVertexArray_ = 0;
    glm::ivec2 size_{0};
    glm::ivec2 bloomSize_{0};

    unsigned int multisampleFramebuffer_ = 0;
    unsigned int multisampleColor_ = 0;
    unsigned int multisampleDepth_ = 0;
    unsigned int resolveFramebuffer_ = 0;
    unsigned int resolveColor_ = 0;
    std::array<unsigned int, 2> bloomFramebuffers_{};
    std::array<unsigned int, 2> bloomColors_{};
};

#include "render/Cubemap.h"

#include "core/Assets.h"

#include <glad/glad.h>
#include <stb_image.h>

#include <array>
#include <stdexcept>
#include <string>

Cubemap::Cubemap(std::string_view assetPrefix)
{
    const std::array<const char*, 6> faceSuffixes = {"px", "nx", "py", "ny", "pz", "nz"};

    glGenTextures(1, &handle_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, handle_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    stbi_set_flip_vertically_on_load(0);

    for (size_t face = 0; face < faceSuffixes.size(); ++face) {
        const std::string relativePath = std::string(assetPrefix) + faceSuffixes[face] + ".png";
        const std::string fullPath = assets::path(relativePath).string();
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load(fullPath.c_str(), &width, &height, &channels, 3);
        if (pixels == nullptr) {
            glDeleteTextures(1, &handle_);
            throw std::runtime_error("Cannot load cubemap face: " + fullPath);
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face), 0, GL_RGB8, width, height, 0, GL_RGB,
            GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Cubemap::~Cubemap()
{
    if (handle_ != 0) {
        glDeleteTextures(1, &handle_);
    }
}

void Cubemap::bind(int unit) const
{
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
    glBindTexture(GL_TEXTURE_CUBE_MAP, handle_);
}

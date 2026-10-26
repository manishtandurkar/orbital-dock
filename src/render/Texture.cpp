#include "render/Texture.h"

#include <glad/glad.h>
#include <stb_image.h>

#include <stdexcept>
#include <utility>

Texture::Texture(const std::filesystem::path& file)
{
    stbi_set_flip_vertically_on_load(1);
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(file.string().c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        throw std::runtime_error("Cannot load texture: " + file.string());
    }

    glGenTextures(1, &handle_);
    glBindTexture(GL_TEXTURE_2D, handle_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);
}

Texture::~Texture()
{
    if (handle_ != 0) {
        glDeleteTextures(1, &handle_);
    }
}

Texture::Texture(Texture&& other) noexcept
    : handle_(std::exchange(other.handle_, 0))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        if (handle_ != 0) {
            glDeleteTextures(1, &handle_);
        }
        handle_ = std::exchange(other.handle_, 0);
    }
    return *this;
}

void Texture::bind(int unit) const
{
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
    glBindTexture(GL_TEXTURE_2D, handle_);
}

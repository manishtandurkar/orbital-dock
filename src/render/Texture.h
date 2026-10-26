#pragma once

#include <filesystem>

class Texture {
public:
    explicit Texture(const std::filesystem::path& file);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    void bind(int unit) const;

private:
    unsigned int handle_ = 0;
};

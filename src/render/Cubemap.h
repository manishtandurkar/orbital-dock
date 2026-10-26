#pragma once

#include <string_view>

class Cubemap {
public:
    explicit Cubemap(std::string_view assetPrefix);
    ~Cubemap();

    Cubemap(const Cubemap&) = delete;
    Cubemap& operator=(const Cubemap&) = delete;

    void bind(int unit) const;

private:
    unsigned int handle_ = 0;
};

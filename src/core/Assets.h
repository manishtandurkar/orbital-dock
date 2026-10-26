#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace assets {

std::filesystem::path path(std::string_view relativePath);

std::string readTextFile(std::string_view relativePath);

}

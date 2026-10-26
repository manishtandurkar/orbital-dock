#include "core/Assets.h"

#include "platform/ExecutablePath.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace assets {

namespace {

std::filesystem::path findAssetRoot()
{
    const std::filesystem::path besideExecutable = platform::executableDirectory() / "assets";
    if (std::filesystem::exists(besideExecutable)) {
        return besideExecutable;
    }
    return std::filesystem::current_path() / "assets";
}

const std::filesystem::path& assetRoot()
{
    static const std::filesystem::path root = findAssetRoot();
    return root;
}

}

std::filesystem::path path(std::string_view relativePath)
{
    return assetRoot() / std::filesystem::u8path(relativePath);
}

std::string readTextFile(std::string_view relativePath)
{
    const std::filesystem::path fullPath = path(relativePath);
    std::ifstream file(fullPath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open asset: " + fullPath.string());
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

}

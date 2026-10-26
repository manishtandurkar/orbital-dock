#include "render/ModelLibrary.h"

const Model& ModelLibrary::get(std::string_view name)
{
    const std::string key(name);
    const auto existing = models_.find(key);
    if (existing != models_.end()) {
        return *existing->second;
    }
    auto model = std::make_unique<Model>("models/" + key + ".obj");
    const Model& reference = *model;
    models_.emplace(key, std::move(model));
    return reference;
}

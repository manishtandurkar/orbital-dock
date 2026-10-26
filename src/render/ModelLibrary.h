#pragma once

#include "render/Model.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class ModelLibrary {
public:
    const Model& get(std::string_view name);

private:
    std::unordered_map<std::string, std::unique_ptr<Model>> models_;
};

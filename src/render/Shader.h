#pragma once

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <string_view>
#include <unordered_map>

class Shader {
public:
    Shader(std::string_view vertexAsset, std::string_view fragmentAsset);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void use() const;

    void setInt(std::string_view name, int value) const;
    void setBool(std::string_view name, bool value) const;
    void setFloat(std::string_view name, float value) const;
    void setVec2(std::string_view name, const glm::vec2& value) const;
    void setVec3(std::string_view name, const glm::vec3& value) const;
    void setVec4(std::string_view name, const glm::vec4& value) const;
    void setMat3(std::string_view name, const glm::mat3& value) const;
    void setMat4(std::string_view name, const glm::mat4& value) const;

private:
    int uniformLocation(std::string_view name) const;

    unsigned int program_ = 0;
    mutable std::unordered_map<std::string, int> uniformCache_;
};

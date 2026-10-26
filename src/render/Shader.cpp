#include "render/Shader.h"

#include "core/Assets.h"
#include "render/GlDebug.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <utility>
#include <vector>

namespace {

GLuint compileStage(GLenum stage, const std::string& source, std::string_view assetName)
{
    const GLuint shader = glCreateShader(stage);
    const char* sourcePointer = source.c_str();
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<size_t>(logLength) + 1, '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Shader compile failed (" + std::string(assetName) + "): " + log.data());
    }
    return shader;
}

GLuint linkProgram(GLuint vertex, GLuint fragment, std::string_view assetName)
{
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDetachShader(program, vertex);
    glDetachShader(program, fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<size_t>(logLength) + 1, '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        glDeleteProgram(program);
        throw std::runtime_error("Shader link failed (" + std::string(assetName) + "): " + log.data());
    }
    return program;
}

}

Shader::Shader(std::string_view vertexAsset, std::string_view fragmentAsset)
{
    const std::string vertexSource = assets::readTextFile(vertexAsset);
    const std::string fragmentSource = assets::readTextFile(fragmentAsset);

    const GLuint vertex = compileStage(GL_VERTEX_SHADER, vertexSource, vertexAsset);
    GLuint fragment = 0;
    try {
        fragment = compileStage(GL_FRAGMENT_SHADER, fragmentSource, fragmentAsset);
        program_ = linkProgram(vertex, fragment, vertexAsset);
    } catch (...) {
        glDeleteShader(vertex);
        if (fragment != 0) {
            glDeleteShader(fragment);
        }
        throw;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    gldebug::checkErrors("Shader::Shader");
}

Shader::~Shader()
{
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

Shader::Shader(Shader&& other) noexcept
    : program_(std::exchange(other.program_, 0))
    , uniformCache_(std::move(other.uniformCache_))
{
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        if (program_ != 0) {
            glDeleteProgram(program_);
        }
        program_ = std::exchange(other.program_, 0);
        uniformCache_ = std::move(other.uniformCache_);
    }
    return *this;
}

void Shader::use() const
{
    glUseProgram(program_);
}

int Shader::uniformLocation(std::string_view name) const
{
    const std::string key(name);
    const auto cached = uniformCache_.find(key);
    if (cached != uniformCache_.end()) {
        return cached->second;
    }
    const int location = glGetUniformLocation(program_, key.c_str());
    uniformCache_.emplace(key, location);
    return location;
}

void Shader::setInt(std::string_view name, int value) const
{
    glUniform1i(uniformLocation(name), value);
}

void Shader::setBool(std::string_view name, bool value) const
{
    glUniform1i(uniformLocation(name), value ? 1 : 0);
}

void Shader::setFloat(std::string_view name, float value) const
{
    glUniform1f(uniformLocation(name), value);
}

void Shader::setVec2(std::string_view name, const glm::vec2& value) const
{
    glUniform2fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(std::string_view name, const glm::vec3& value) const
{
    glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(std::string_view name, const glm::vec4& value) const
{
    glUniform4fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat3(std::string_view name, const glm::mat3& value) const
{
    glUniformMatrix3fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat4(std::string_view name, const glm::mat4& value) const
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

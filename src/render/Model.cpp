#include "render/Model.h"

#include "core/Assets.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <stdexcept>

namespace {

glm::vec3 toVec3(const aiColor3D& color)
{
    return {color.r, color.g, color.b};
}

std::vector<Vertex> extractVertices(const aiMesh& mesh)
{
    std::vector<Vertex> vertices;
    vertices.reserve(mesh.mNumVertices);
    for (unsigned int i = 0; i < mesh.mNumVertices; ++i) {
        Vertex vertex{};
        vertex.position = {mesh.mVertices[i].x, mesh.mVertices[i].y, mesh.mVertices[i].z};
        if (mesh.HasNormals()) {
            vertex.normal = {mesh.mNormals[i].x, mesh.mNormals[i].y, mesh.mNormals[i].z};
        }
        if (mesh.HasTextureCoords(0)) {
            vertex.uv = {mesh.mTextureCoords[0][i].x, mesh.mTextureCoords[0][i].y};
        }
        vertices.push_back(vertex);
    }
    return vertices;
}

std::vector<std::uint32_t> extractIndices(const aiMesh& mesh)
{
    std::vector<std::uint32_t> indices;
    indices.reserve(static_cast<size_t>(mesh.mNumFaces) * 3);
    for (unsigned int i = 0; i < mesh.mNumFaces; ++i) {
        const aiFace& face = mesh.mFaces[i];
        if (face.mNumIndices != 3) {
            continue;
        }
        indices.insert(indices.end(), {face.mIndices[0], face.mIndices[1], face.mIndices[2]});
    }
    return indices;
}

}

Model::Model(std::string_view assetPath)
{
    const std::filesystem::path fullPath = assets::path(assetPath);
    Assimp::Importer importer;
    const unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices
        | aiProcess_ImproveCacheLocality | aiProcess_SortByPType;
    const aiScene* scene = importer.ReadFile(fullPath.string(), flags);
    if (scene == nullptr || scene->mRootNode == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0) {
        throw std::runtime_error("Cannot load model " + fullPath.string() + ": " + importer.GetErrorString());
    }

    const std::filesystem::path modelDirectory = fullPath.parent_path();
    boundsMin_ = glm::vec3(std::numeric_limits<float>::max());
    boundsMax_ = glm::vec3(std::numeric_limits<float>::lowest());

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        const aiMesh& sourceMesh = *scene->mMeshes[meshIndex];
        if ((sourceMesh.mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0) {
            continue;
        }

        Material material;
        const aiMaterial& sourceMaterial = *scene->mMaterials[sourceMesh.mMaterialIndex];
        aiColor3D color(0.0f, 0.0f, 0.0f);
        if (sourceMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            material.diffuseColor = toVec3(color);
        }
        if (sourceMaterial.Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            material.specularColor = toVec3(color);
        }
        if (sourceMaterial.Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
            material.emissiveColor = toVec3(color);
        }
        float shininess = 0.0f;
        if (sourceMaterial.Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS && shininess > 0.0f) {
            material.shininess = shininess;
        }
        aiString texturePath;
        if (sourceMaterial.GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            const std::filesystem::path textureFile = (modelDirectory / std::filesystem::u8path(texturePath.C_Str())).lexically_normal();
            material.diffuseMap = loadTexture(textureFile.string());
        }

        Mesh mesh(extractVertices(sourceMesh), extractIndices(sourceMesh));
        boundsMin_ = glm::min(boundsMin_, mesh.boundsMin());
        boundsMax_ = glm::max(boundsMax_, mesh.boundsMax());
        parts_.push_back({std::move(mesh), material});
    }

    if (parts_.empty()) {
        throw std::runtime_error("Model has no triangle meshes: " + fullPath.string());
    }
    boundingRadius_ = std::max(glm::length(boundsMin_), glm::length(boundsMax_));
}

const Texture* Model::loadTexture(const std::string& path)
{
    const auto existing = textures_.find(path);
    if (existing != textures_.end()) {
        return existing->second.get();
    }
    auto texture = std::make_unique<Texture>(std::filesystem::u8path(path));
    const Texture* raw = texture.get();
    textures_.emplace(path, std::move(texture));
    return raw;
}

const std::vector<Model::Part>& Model::parts() const
{
    return parts_;
}

const glm::vec3& Model::boundsMin() const
{
    return boundsMin_;
}

const glm::vec3& Model::boundsMax() const
{
    return boundsMax_;
}

float Model::boundingRadius() const
{
    return boundingRadius_;
}

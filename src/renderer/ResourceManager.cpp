#include "ResourceManager.h"
#include <iostream>

namespace Sylva {

ResourceManager::ResourceManager() {
}

ResourceManager::~ResourceManager() {
    Clear();
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string& vertexPath, const std::string& fragmentPath) {
    // Create a key for the shader pair
    std::string key = MakeShaderKey(vertexPath, fragmentPath);
    
    // Check if shader is already loaded
    auto it = m_ShaderCache.find(key);
    if (it != m_ShaderCache.end()) {
        return it->second;
    }
    
    // Not found, load the shader
    auto shader = std::make_shared<Shader>();
    if (!shader->LoadFromFiles(vertexPath, fragmentPath)) {
        std::cerr << "Failed to load shader: " << vertexPath << " / " << fragmentPath << std::endl;
        return nullptr;
    }
    
    // Store in cache and return
    m_ShaderCache[key] = shader;
    return shader;
}

std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& path) {
    // Check if texture is already loaded
    auto it = m_TextureCache.find(path);
    if (it != m_TextureCache.end()) {
        return it->second;
    }
    
    // Not found, load the texture
    auto texture = std::make_shared<Texture>();
    if (!texture->LoadFromFile(path)) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return nullptr;
    }
    
    // Store in cache and return
    m_TextureCache[path] = texture;
    return texture;
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::string& path) {
    // Check if model is already loaded
    auto it = m_ModelCache.find(path);
    if (it != m_ModelCache.end()) {
        return it->second;
    }
    
    // Not found, load the model
    auto model = std::make_shared<Model>();
    if (!model->LoadFromFile(path)) {
        std::cerr << "Failed to load model: " << path << std::endl;
        return nullptr;
    }
    
    // Store in cache and return
    m_ModelCache[path] = model;
    return model;
}

void ResourceManager::Clear() {
    // Clear all caches - resources will be deleted when shared_ptrs are removed
    m_ShaderCache.clear();
    m_TextureCache.clear();
    m_ModelCache.clear();
}

std::string ResourceManager::MakeShaderKey(const std::string& vertexPath, const std::string& fragmentPath) {
    return vertexPath + ":" + fragmentPath;
}

std::unique_ptr<Mesh> ResourceManager::CreateDynamicMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    auto mesh = std::make_unique<Mesh>();
    // Use the same vertex layout parameters as the original Renderer::CreateMesh
    // stride: 6, posOffset: 0, normOffset: -1, texOffset: -1, colOffset: 3
    mesh->SetVertexData(vertices, indices, 6, 0, -1, -1, 3);
    return mesh;
}

} // namespace Sylva 
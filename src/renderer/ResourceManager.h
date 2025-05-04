#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "Shader.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

namespace Sylva {

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();
    
    // Get or load a shader
    std::shared_ptr<Shader> GetShader(const std::string& vertexPath, const std::string& fragmentPath);
    
    // Get or load a texture
    std::shared_ptr<Texture> GetTexture(const std::string& path);
    
    // Get or load a model
    std::shared_ptr<Model> GetModel(const std::string& path);
    
    // Create a mesh dynamically from vertex/index data
    std::unique_ptr<Mesh> CreateDynamicMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    
    // Clear all resources (useful for reloading)
    void Clear();
    
private:
    // Resource caches
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_ShaderCache;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_TextureCache;
    std::unordered_map<std::string, std::shared_ptr<Model>> m_ModelCache;
    
    // Helper to create shader path key
    std::string MakeShaderKey(const std::string& vertexPath, const std::string& fragmentPath);
};

} // namespace Sylva 
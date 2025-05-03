#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

#include "../platform/Platform.h"
#include "Camera.h"
#include "CameraController.h"
#include "Shader.h"
#include "Texture.h"
#include "Mesh.h"
#include "Model.h"
#include "ResourceManager.h"

namespace Sylva {

// Forward declarations
class Platform;
class Camera;
class CameraController;
class ResourceManager;

// Main renderer class
class Renderer {
public:
    Renderer();
    ~Renderer();
    
    bool Initialize();
    void BeginFrame();
    void RenderScene();
    void EndFrame();
    void Shutdown();
    
    void SetClearColor(const glm::vec4& color);
    
    // Camera methods
    Camera* GetCamera() const { return m_Camera; }
    CameraController* GetCameraController() const { return m_CameraController; }
    void SetupCamera(Platform* platform);
    
    // Resource management (legacy methods that will use ResourceManager internally)
    // These are kept for backward compatibility but should be phased out
    Shader* LoadShader(const std::string& vertexPath, const std::string& fragmentPath);
    Mesh* CreateMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    Texture* LoadTexture(const std::string& path);
    Model* LoadModel(const std::string& path);
    
    // Get the resource manager
    ResourceManager* GetResourceManager() const { return m_ResourceManager.get(); }
    
private:
    void CreateTriangle(); // Temporary test function for Phase 1
    
    // Renderer state
    glm::vec4 m_ClearColor;
    
    // Resource manager
    std::unique_ptr<ResourceManager> m_ResourceManager;
    
    // Test objects for Phase 1 (these should be moved to a Scene class later)
    Shader* m_BasicShader;
    Mesh* m_TestTriangle;
    
    // Model rendering objects
    Shader* m_TexturedShader;
    Model* m_TestModel;
    
    // Camera
    Camera* m_Camera;
    CameraController* m_CameraController;
};

} 
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

#include "../platform/Platform.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "Mesh.h"
#include "Model.h"
#include "ResourceManager.h"
#include "DebugDrawing.h"

namespace Sylva {

// Forward declarations
class Platform;
class Camera;
class ResourceManager;
class CameraController;
class CameraDebug;

// Main renderer class
class Renderer {
public:
    Renderer();
    ~Renderer();
    
    bool Initialize();
    void BeginFrame();
    void EndFrame();
    void Shutdown();
    
    void SetClearColor(const glm::vec4& color);
    
    // Camera methods
    Camera* GetCamera() const { return m_Camera.get(); }
    
    // Get the active camera (may be controlled by a CameraController)
    Camera* GetActiveCamera() const { return m_ActiveCamera ? m_ActiveCamera : m_Camera.get(); }
    
    // Set the active camera (for example, from a CameraController)
    void SetActiveCamera(Camera* camera) { m_ActiveCamera = camera; }
    
    // Resource management (legacy methods that will use ResourceManager internally)
    // These are kept for backward compatibility but should be phased out
    Shader* LoadShader(const std::string& vertexPath, const std::string& fragmentPath);
    Texture* LoadTexture(const std::string& path);
    Model* LoadModel(const std::string& path);
    
    // Get the resource manager
    ResourceManager* GetResourceManager() const { return m_ResourceManager.get(); }
    
    // Debug drawing methods
    void DrawDebugLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f));
    void DrawDebugCircle(const glm::vec3& center, float radius, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f), int segments = 36);
    void DrawDebugSphere(const glm::vec3& center, float radius, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f), int segments = 36);
    void DrawDebugBox(const glm::vec3& center, const glm::vec3& size, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f));
    void DrawDebugCoordinateAxes(const glm::vec3& position, float scale = 1.0f);
    
    // Enable/disable debug drawing
    void SetDebugDrawingEnabled(bool enabled) { m_DebugDrawingEnabled = enabled; }
    bool IsDebugDrawingEnabled() const { return m_DebugDrawingEnabled; }
    
    // Register a CameraDebug instance
    void RegisterCameraDebug(CameraDebug* cameraDebug) { m_CameraDebug = cameraDebug; }
    
private:
    // Renderer state
    glm::vec4 m_ClearColor;
    
    // Resource manager
    std::unique_ptr<ResourceManager> m_ResourceManager;
    
    // Camera
    std::unique_ptr<Camera> m_Camera;
    
    // Active camera (non-owning pointer, may point to camera owned by CameraController)
    Camera* m_ActiveCamera;
    
    // Debug drawing
    std::unique_ptr<DebugDrawing> m_DebugDrawing;
    bool m_DebugDrawingEnabled;
    
    // Camera debug (non-owning pointer, owned by Core)
    CameraDebug* m_CameraDebug;
};

} 
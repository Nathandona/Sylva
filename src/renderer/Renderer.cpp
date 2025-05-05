#include "Renderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp> // For glm::translate, rotate, etc.
#include "CameraDebug.h"
#include "../core/Logger.h"

namespace Sylva {

Renderer::Renderer()
    : m_ClearColor(0.53f, 0.81f, 0.92f, 1.0f)  // Light blue sky color
    , m_ActiveCamera(nullptr)
    , m_DebugDrawingEnabled(false)
    , m_CameraDebug(nullptr)
{
    // Initialize pointers to nullptr in the initializer list
}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize() {
    LOG_INFO("Initializing Renderer...");
    
    // Create resource manager
    m_ResourceManager = std::make_unique<ResourceManager>();
    
    // Set OpenGL state
    glEnable(GL_DEPTH_TEST);
    // Temporarily disable face culling for debugging visibility issues
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    
    // Initialize camera using std::make_unique
    m_Camera = std::make_unique<Camera>(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    m_Camera->SetPosition(glm::vec3(0.0f, 0.0f, 3.0f));
    
    // Initialize debug drawing
    m_DebugDrawing = std::make_unique<DebugDrawing>();
    if (!m_DebugDrawing->Initialize()) {
        LOG_ERROR("Failed to initialize debug drawing");
        return false;
    }
    
    return true;
}

void Renderer::BeginFrame() {
    // Clear the color and depth buffer
    glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Clear debug drawing data from previous frame
    if (m_DebugDrawing) {
        m_DebugDrawing->Clear();
    }
}

void Renderer::EndFrame() {
    // Render debug visualizations after regular scene rendering
    if (m_DebugDrawingEnabled && m_DebugDrawing) {
        // Get the view and projection matrices from the active camera
        const Camera* activeCamera = GetActiveCamera();
        if (activeCamera) {
            glm::mat4 viewMatrix = activeCamera->GetViewMatrix();
            glm::mat4 projectionMatrix = activeCamera->GetProjectionMatrix();
            
            // Render debug drawings
            m_DebugDrawing->Render(viewMatrix, projectionMatrix);
        }
    }
    
    // Update and render camera debug visualizations if enabled
    if (m_CameraDebug && m_CameraDebug->IsEnabled()) {
        m_CameraDebug->DrawDebugVisualizations();
    }
}

void Renderer::Shutdown() {
    // Shutdown debug drawing
    if (m_DebugDrawing) {
        m_DebugDrawing->Shutdown();
    }
    
    // ResourceManager and Camera will be automatically deleted by their unique_ptrs
}

void Renderer::SetClearColor(const glm::vec4& color) {
    m_ClearColor = color;
}

Shader* Renderer::LoadShader(const std::string& vertexPath, const std::string& fragmentPath) {
    // Use ResourceManager to load the shader
    std::shared_ptr<Shader> shader = m_ResourceManager->GetShader(vertexPath, fragmentPath);
    
    // Return raw pointer for backward compatibility (ResourceManager maintains ownership)
    return shader.get();
}

Texture* Renderer::LoadTexture(const std::string& path) {
    // Use ResourceManager to load the texture
    std::shared_ptr<Texture> texture = m_ResourceManager->GetTexture(path);
    
    // Return raw pointer for backward compatibility (ResourceManager maintains ownership)
    return texture.get();
}

Model* Renderer::LoadModel(const std::string& path) {
    // Use ResourceManager to load the model
    std::shared_ptr<Model> model = m_ResourceManager->GetModel(path);
    
    // Return raw pointer for backward compatibility (ResourceManager maintains ownership)
    return model.get();
}

// Debug drawing forwarding methods
void Renderer::DrawDebugLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
    if (!m_DebugDrawingEnabled || !m_DebugDrawing) return;
    
    m_DebugDrawing->DrawLine(start, end, color);
}

void Renderer::DrawDebugCircle(const glm::vec3& center, float radius, const glm::vec3& color, int segments) {
    if (!m_DebugDrawingEnabled || !m_DebugDrawing) return;
    
    m_DebugDrawing->DrawCircle(center, radius, color, segments);
}

void Renderer::DrawDebugSphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments) {
    if (!m_DebugDrawingEnabled || !m_DebugDrawing) return;
    
    m_DebugDrawing->DrawSphere(center, radius, color, segments);
}

void Renderer::DrawDebugBox(const glm::vec3& center, const glm::vec3& size, const glm::vec3& color) {
    if (!m_DebugDrawingEnabled || !m_DebugDrawing) return;
    
    m_DebugDrawing->DrawBox(center, size, color);
}

void Renderer::DrawDebugCoordinateAxes(const glm::vec3& position, float scale) {
    if (!m_DebugDrawingEnabled || !m_DebugDrawing) return;
    
    m_DebugDrawing->DrawCoordinateAxes(position, scale);
}

} // namespace Sylva 
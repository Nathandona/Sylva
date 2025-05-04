#include "Renderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp> // For glm::translate, rotate, etc.

namespace Sylva {

Renderer::Renderer()
    : m_ClearColor(0.53f, 0.81f, 0.92f, 1.0f)  // Light blue sky color
{
    // Initialize pointers to nullptr in the initializer list
}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize() {
    std::cout << "Initializing Renderer..." << std::endl;
    
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
    
    return true;
}

void Renderer::BeginFrame() {
    // Clear the color and depth buffer
    glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame() {
    // Currently nothing to do here
}

void Renderer::Shutdown() {
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

} // namespace Sylva 
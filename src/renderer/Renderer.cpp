#include "Renderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp> // For glm::translate, rotate, etc.

namespace Sylva {

Renderer::Renderer()
    : m_ClearColor(0.53f, 0.81f, 0.92f, 1.0f)  // Light blue sky color
    , m_ResourceManager(nullptr)
    , m_BasicShader(nullptr)
    , m_TestTriangle(nullptr)
    , m_TexturedShader(nullptr)
    , m_TestModel(nullptr)
    , m_Camera(nullptr)
    , m_CameraController(nullptr)
{
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
    
    // Create a basic shader for our triangle
    m_BasicShader = LoadShader("assets/shaders/basic.vert", "assets/shaders/basic.frag");
    if (!m_BasicShader) {
        std::cerr << "Failed to load basic shaders" << std::endl;
        return false;
    }
    
    // Create shader for textured models
    m_TexturedShader = LoadShader("assets/shaders/textured.vert", "assets/shaders/textured.frag");
    if (!m_TexturedShader) {
        std::cerr << "Failed to load textured shaders" << std::endl;
        return false;
    }
    
    // Load a test model
    m_TestModel = LoadModel("assets/models/cube.obj");
    if (!m_TestModel) {
        std::cerr << "Failed to load cube model" << std::endl;
        // Continue anyway - this isn't fatal
    }
    
    // Initialize camera (platform-specific setup is done later)
    m_Camera = new Camera(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    m_Camera->SetPosition(glm::vec3(0.0f, 0.0f, 3.0f));
    
    return true;
}

void Renderer::BeginFrame() {
    // Clear the color and depth buffer
    glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetupCamera(Platform* platform) {
    if (platform) {
        // Create a camera controller with the platform for input
        m_CameraController = new CameraController(m_Camera, platform);
        
        // Set an initial position above the terrain
        m_CameraController->SetTargetPosition(glm::vec3(0.0f, 10.0f, 0.0f));
        
        // Create test triangle for Phase 1
        CreateTriangle();
        
        return;
    }
    
    std::cerr << "Cannot set up camera controller without platform!" << std::endl;
}

void Renderer::RenderScene() {
    // This is a simple test function that renders our test objects
    // In a real engine, this would be more sophisticated and driven by a scene graph
    
    if (m_BasicShader && m_TestTriangle) {
        // Use the shader
        m_BasicShader->Use();
        
        // Set view and projection matrices
        m_BasicShader->SetMat4("view", m_Camera->GetViewMatrix());
        m_BasicShader->SetMat4("projection", m_Camera->GetProjectionMatrix());
        
        // Create a model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f));
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        m_BasicShader->SetMat4("model", model);
        
        // Draw the triangle
        m_TestTriangle->Draw();
    }
    
    if (m_TexturedShader && m_TestModel) {
        // Use the shader
        m_TexturedShader->Use();
        
        // Set view and projection matrices
        m_TexturedShader->SetMat4("view", m_Camera->GetViewMatrix());
        m_TexturedShader->SetMat4("projection", m_Camera->GetProjectionMatrix());
        
        // Create a model matrix for the cube
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(3.0f, 0.0f, -5.0f));
        model = glm::rotate(model, (float)glfwGetTime() * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
        m_TexturedShader->SetMat4("model", model);
        
        // Draw the model
        m_TestModel->Draw(m_TexturedShader);
    }
}

void Renderer::EndFrame() {
    // Currently nothing to do here
}

void Renderer::Shutdown() {
    // Note: we don't need to delete shaders and models loaded through ResourceManager
    // but we still need to clean up any raw pointers we have
    
    delete m_TestTriangle;
    m_TestTriangle = nullptr;
    
    m_BasicShader = nullptr;
    m_TexturedShader = nullptr;
    m_TestModel = nullptr;
    
    delete m_CameraController;
    m_CameraController = nullptr;
    
    delete m_Camera;
    m_Camera = nullptr;
    
    // ResourceManager will be automatically deleted by the unique_ptr
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

Mesh* Renderer::CreateMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    // For now, we still create meshes directly since they're not managed by ResourceManager
    Mesh* mesh = new Mesh();
    mesh->SetVertexData(vertices, indices, 6, 0, -1, -1, 3);
    return mesh;
}

void Renderer::CreateTriangle() {
    // Define vertices for a simple triangle with position (xyz) and color (rgb)
    std::vector<float> vertices = {
        // positions         // colors
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // top
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom left
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // bottom right
    };
    
    // Define indices
    std::vector<unsigned int> indices = {
        0, 1, 2  // first triangle
    };
    
    // Create the mesh
    m_TestTriangle = new Mesh();
    m_TestTriangle->SetVertexData(vertices, indices, 6, 0, -1, -1, 3);
}

// New renderer functions for textures and models
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
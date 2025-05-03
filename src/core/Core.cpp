#include "Core.h"
#include <iostream>

namespace Sylva {

Core::Core() 
    : m_Running(false)
    , m_DeltaTime(0.0f)
{
}

Core::~Core() {
    Shutdown();
}

bool Core::Initialize() {
    std::cout << "Initializing Sylva Engine..." << std::endl;
    
    // Initialize the platform layer (window, input)
    if (!m_Platform.Initialize("Sylva", 1280, 720)) {
        std::cerr << "Failed to initialize Platform" << std::endl;
        return false;
    }
    
    // Set the window icon
    // The icon path is relative to the working directory when the application is run
    if (!m_Platform.SetWindowIcon("assets/icon.png")) {
        std::cerr << "Warning: Failed to set window icon" << std::endl;
        // Not a critical error, continue initialization
    }
    
    // Initialize the renderer
    if (!m_Renderer.Initialize()) {
        std::cerr << "Failed to initialize Renderer" << std::endl;
        return false;
    }
    
    // Position the camera for a Cube World style view
    m_Renderer.GetCamera()->SetPosition(glm::vec3(0.0f, 5.0f, 5.0f));
    m_Renderer.GetCamera()->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    
    // Setup camera with platform
    m_Renderer.SetupCamera(&m_Platform);
    
    // Initialize the world
    if (!m_World.Initialize(&m_Renderer, &m_Platform)) {
        std::cerr << "Failed to initialize World" << std::endl;
        return false;
    }
    
    m_Running = true;
    return true;
}

void Core::Run() {
    std::cout << "Starting main game loop..." << std::endl;
    
    float lastFrameTime = m_Platform.GetTime();
    
    // Main game loop
    while (m_Running) {
        // Calculate delta time
        float currentTime = m_Platform.GetTime();
        m_DeltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        
        // Process window events and input
        m_Platform.PollEvents();
        
        // Check if window should close
        if (m_Platform.WindowShouldClose()) {
            m_Running = false;
            continue;
        }
        
        // Update camera
        if (m_Renderer.GetCameraController()) {
            m_Renderer.GetCameraController()->Update(m_DeltaTime);
        }
        
        // Update world
        m_World.Update(m_DeltaTime);
        
        // Begin new frame
        m_Renderer.BeginFrame();
        
        // Render the world
        m_World.Render();
        
        // Render the scene (models, etc. not part of world)
        m_Renderer.RenderScene();
        
        // End frame and swap buffers
        m_Renderer.EndFrame();
        m_Platform.SwapBuffers();
    }
}

void Core::Shutdown() {
    std::cout << "Shutting down Sylva Engine..." << std::endl;
    
    // Shutdown in reverse order of initialization
    m_Renderer.Shutdown();
    m_Platform.Shutdown();
}

} 
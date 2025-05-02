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
    
    // Initialize the renderer
    if (!m_Renderer.Initialize()) {
        std::cerr << "Failed to initialize Renderer" << std::endl;
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
        
        // Begin new frame
        m_Renderer.BeginFrame();
        
        // TODO: Update game systems
        
        // Render the scene
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
#include "Core.h"
#include <iostream>

namespace Sylva {

Core::Core() 
    : m_Running(false)
    , m_DeltaTime(0.0f)
    , m_CameraController(nullptr)
{
}

Core::~Core() {
    Shutdown();
}

bool Core::Initialize() {
    std::cout << "Initializing Sylva Engine..." << std::endl;
    
    // Initialize platform (window and input)
    if (!m_Platform.Initialize("Sylva", 1280, 720)) {
        std::cerr << "Failed to initialize platform!" << std::endl;
        return false;
    }
    
    // Initialize renderer
    if (!m_Renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return false;
    }
    
    // Initialize world
    if (!m_World.Initialize(&m_Renderer, &m_Platform)) {
        std::cerr << "Failed to initialize world!" << std::endl;
        return false;
    }
    
    // Get camera from renderer
    Camera* camera = m_Renderer.GetCamera();
    if (!camera) {
        std::cerr << "Failed to get camera from renderer!" << std::endl;
        return false;
    }
    
    // Create camera controller
    m_CameraController = std::make_unique<CameraController>(camera, &m_Platform);
    
    // Configure camera controller for third-person view
    if (m_CameraController) {
        // Set initial orbit distance (how far behind the player)
        m_CameraController->SetOrbitDistance(4.0f);
        
        // Set vertical offset (how far above the player) - ensure this is enough to see the player
        m_CameraController->SetVerticalOffset(1.5f);
        
        // Set smoothing factor (higher = faster camera movement)
        m_CameraController->SetSmoothingFactor(5.0f);
        
        // Set collision buffer (distance from collision point to place camera)
        m_CameraController->SetCollisionBuffer(0.2f);
        
        // Set the world reference for collision detection
        m_CameraController->SetWorld(&m_World);
        
        // Get player information
        glm::vec3 playerPos = m_World.GetPlayer().GetPosition();
        float playerYaw = m_World.GetPlayer().GetYaw();
        
        std::cout << "Initial player position: x=" << playerPos.x << ", y=" << playerPos.y << ", z=" << playerPos.z << std::endl;
        std::cout << "Initial player yaw: " << playerYaw << std::endl;
        
        // Set initial target position and yaw
        m_CameraController->SetTargetPosition(playerPos);
        m_CameraController->SetTargetYaw(playerYaw);
        
        // Force an immediate camera update to position it correctly at startup
        m_CameraController->Update(0.016f); // Using a small delta time for the first update
        
        // Call the update a second time to make sure camera is fully stabilized
        m_CameraController->Update(0.016f);

        // Log initial camera position after setup
        glm::vec3 camPos = camera->GetPosition();
        std::cout << "Initial camera position: x=" << camPos.x << ", y=" << camPos.y << ", z=" << camPos.z << std::endl;
    }
    
    // Set up running state
    m_Running = true;
    
    std::cout << "Sylva Engine initialized successfully." << std::endl;
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
        
        // Get camera for the frame
        Camera* camera = m_CameraController ? m_CameraController->GetCamera() : nullptr;
        
        // Update world (which updates the player)
        m_World.Update(m_DeltaTime, camera);
        
        // Update camera target position based on player's position
        if (m_CameraController) {
            // Get latest player state AFTER world update
            glm::vec3 playerPos = m_World.GetPlayer().GetPosition();
            float playerYaw = m_World.GetPlayer().GetYaw();
            
            // Log player position only on significant change (to avoid console spam)
            static glm::vec3 lastLoggedPos(0.0f);
            static float lastLoggedYaw = 0.0f;
            if (glm::distance(playerPos, lastLoggedPos) > 1.0f || std::abs(playerYaw - lastLoggedYaw) > 10.0f) {
                std::cout << "Player position updated: x=" << playerPos.x << ", y=" << playerPos.y << ", z=" << playerPos.z << std::endl;
                std::cout << "Player yaw: " << playerYaw << std::endl;
                
                // Also log camera position when player position is logged
                glm::vec3 camPos = m_CameraController->GetCamera()->GetPosition();
                std::cout << "Camera position: x=" << camPos.x << ", y=" << camPos.y << ", z=" << camPos.z << std::endl;
                
                lastLoggedPos = playerPos;
                lastLoggedYaw = playerYaw;
            }
            
            // Provide player state to camera controller
            m_CameraController->SetTargetPosition(playerPos);
            m_CameraController->SetTargetYaw(playerYaw);
            
            // Update camera controller
            m_CameraController->Update(m_DeltaTime);
        }
        
        // Begin new frame
        m_Renderer.BeginFrame();
        
        // Render the world
        m_World.Render(camera);
        
        // End frame and swap buffers
        m_Renderer.EndFrame();
        m_Platform.SwapBuffers();
    }
}

void Core::Shutdown() {
    std::cout << "Shutting down Sylva Engine..." << std::endl;
    
    // Shutdown in reverse order of initialization
    m_CameraController.reset(); // Ensure controller is destroyed before renderer
    m_Renderer.Shutdown();
    m_Platform.Shutdown();
}

} 
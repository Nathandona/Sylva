#include "Core.h"
#include <iostream>

namespace Sylva {

Core::Core() 
    : m_Running(false)
    , m_DeltaTime(0.0f)
    , m_InputManager(&m_Platform)
    , m_CameraController(nullptr)
{
}

Core::~Core() {
    Shutdown();
}

bool Core::Initialize() {
    std::cout << "Initializing Sylva Engine..." << std::endl;
    
    // Initialize platform (window and input)
    if (!m_Platform.Initialize("Sylva", 1920, 1080)) {
        std::cerr << "Failed to initialize platform!" << std::endl;
        return false;
    }
    
    // Initialize input manager with default mappings
    m_InputManager.Initialize();
    
    // Initialize renderer
    if (!m_Renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return false;
    }
    
    // Initialize world
    if (!m_World.Initialize(&m_Renderer, &m_Platform, &m_InputManager)) {
        std::cerr << "Failed to initialize world!" << std::endl;
        return false;
    }
    
    // Get camera from renderer
    Camera* camera = m_Renderer.GetCamera();
    if (!camera) {
        std::cerr << "Failed to get camera from renderer!" << std::endl;
        return false;
    }
    
    // Create camera controller with references instead of pointers
    m_CameraController = std::make_unique<CameraController>(*camera, m_Platform, m_InputManager);
    
    // Configure camera controller
    if (m_CameraController) {
        // Set the camera mode to ThirdPersonOrbit
        m_CameraController->SetCameraMode(CameraController::CameraMode::ThirdPersonOrbit);
        
        // Set orbit distance (how far behind the player)
        m_CameraController->SetOrbitDistance(7.0f);
        
        // Set min and max zoom distances
        m_CameraController->SetMinOrbitDistance(1.0f);
        m_CameraController->SetMaxOrbitDistance(15.0f);
        
        // Set vertical offset (how far above the player)
        m_CameraController->SetVerticalOffset(2.0f);
        
        // Set look-at offset (vertical offset for camera target)
        m_CameraController->SetLookAtOffset(1.0f);
        
        // Set shoulder offset (0.0f for centered camera)
        m_CameraController->SetShoulderOffset(0.0f);
        
        // Set smoothing factors
        m_CameraController->SetSmoothingFactor(5.0f);
        
        // Connect world for collision detection
        m_CameraController->SetWorld(&m_World);
        
        // Enable collision detection
        m_CameraController->SetCollisionDetection(true);
        
        // Set the player as the target for the camera
        m_CameraController->SetPlayerTarget(&m_World.GetPlayer());
        
        // Set the CameraController's camera as the active camera in the renderer
        // This allows the Player to access the camera for movement direction
        m_Renderer.SetActiveCamera(&m_CameraController->GetCamera());
        
        // Force an immediate camera update to position it correctly at startup
        m_CameraController->Update(0.016f); // Using a small delta time for the first update
        
        // Call the update a second time to make sure camera is fully stabilized
        m_CameraController->Update(0.016f);

        // Log initial camera position after setup
        glm::vec3 camPos = camera->GetPosition();
        std::cout << "Initial camera position: x=" << camPos.x << ", y=" << camPos.y << ", z=" << camPos.z << std::endl;
        std::cout << "Camera mode: Third-Person Orbit Camera" << std::endl;
        std::cout << "Press C to toggle between camera modes." << std::endl;
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
        
        // Get camera for the frame (now using reference)
        Camera& camera = m_CameraController ? m_CameraController->GetCamera() : *m_Renderer.GetCamera();
        
        // Update world (which updates the player)
        m_World.Update(m_DeltaTime, &camera);
        
        // Update camera controller (player position is now handled inside the controller)
        if (m_CameraController) {
            // Log player and camera positions periodically (for debugging only)
            static float logTimer = 0.0f;
            logTimer += m_DeltaTime;
            if (logTimer > 5.0f) { // Log every 5 seconds
                Player* player = m_CameraController->GetPlayerTarget();
                if (player) {
                    glm::vec3 playerPos = player->GetPosition();
                    float playerYaw = player->GetYaw();
                    glm::vec3 camPos = camera.GetPosition();
                    
                    std::cout << "Player position: x=" << playerPos.x << ", y=" << playerPos.y << ", z=" << playerPos.z << ", yaw=" << playerYaw << std::endl;
                    std::cout << "Camera position: x=" << camPos.x << ", y=" << camPos.y << ", z=" << camPos.z << std::endl;
                    std::cout << "Orbit distance: " << m_CameraController->GetOrbitDistance() << std::endl;
                }
                logTimer = 0.0f;
            }
            
            // Update camera controller - this now automatically gets player position
            m_CameraController->Update(m_DeltaTime);
        }
        
        // Begin new frame
        m_Renderer.BeginFrame();
        
        // Render the world
        m_World.Render(&camera);
        
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
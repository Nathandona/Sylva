#include "Core.h"
#include <iostream>
#include "Logger.h"
#include "Config.h"
#include <GLFW/glfw3.h>

namespace Sylva {

Core::Core() 
    : m_Running(false)
    , m_DeltaTime(0.0f)
    , m_InputManager(&m_Platform)
    , m_CameraController(nullptr)
    , m_CameraDebug(nullptr)
{
}

Core::~Core() {
    Shutdown();
}

bool Core::Initialize() {
    LOG_INFO("Initializing Sylva Engine...");
    
    // Initialize platform (window and input)
    if (!m_Platform.Initialize("Sylva", 1920, 1080)) {
        LOG_FATAL("Failed to initialize platform!");
        return false;
    }
    
    // Initialize input manager with default mappings
    m_InputManager.Initialize();
    
    // Initialize renderer
    if (!m_Renderer.Initialize()) {
        LOG_FATAL("Failed to initialize renderer!");
        return false;
    }
    
    // Initialize world
    if (!m_World.Initialize(&m_Renderer, &m_Platform, &m_InputManager)) {
        LOG_FATAL("Failed to initialize world!");
        return false;
    }
    
    // Get camera from renderer
    Camera* camera = m_Renderer.GetCamera();
    if (!camera) {
        LOG_FATAL("Failed to get camera from renderer!");
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
        
        // Create camera debug system
        m_CameraDebug = std::make_unique<CameraDebug>(camera, m_CameraController.get(), &m_Renderer);
        
        // Apply camera debug settings from config
        if (m_CameraDebug) {
            // Enable camera debugging if specified in config
            bool enableCameraDebug = Config::GetInstance().Get<bool>("enableCameraDebug", false);
            m_CameraDebug->SetEnabled(enableCameraDebug);
            
            // Set debug visualization options
            bool showOrbitPath = Config::GetInstance().Get<bool>("showOrbitPath", true);
            m_CameraDebug->SetShowOrbitPath(showOrbitPath);
            
            bool showDebugLines = Config::GetInstance().Get<bool>("showDebugLines", true);
            m_CameraDebug->SetShowDebugLines(showDebugLines);
            
            // Set logging interval
            float logInterval = Config::GetInstance().Get<float>("cameraDebugLogInterval", 0.2f);
            m_CameraDebug->SetLogInterval(logInterval);
            
            // Start recording if specified in config
            bool logCameraPositions = Config::GetInstance().Get<bool>("logCameraPositions", false);
            if (logCameraPositions) {
                m_CameraDebug->StartRecording();
            }
            
            LOG_INFO("Camera debugging ", enableCameraDebug ? "enabled" : "disabled",
                    " (showOrbitPath=", showOrbitPath,
                    ", showDebugLines=", showDebugLines,
                    ", logInterval=", logInterval, ")");
        }
        
        // Force an immediate camera update to position it correctly at startup
        m_CameraController->Update(0.016f); // Using a small delta time for the first update
        
        // Call the update a second time to make sure camera is fully stabilized
        m_CameraController->Update(0.016f);

        // Log initial camera position after setup
        glm::vec3 camPos = camera->GetPosition();
        LOG_INFO("Initial camera position: x=", camPos.x, ", y=", camPos.y, ", z=", camPos.z);
        LOG_INFO("Camera mode: Third-Person Orbit Camera");
        LOG_INFO("Press C to toggle between camera modes.");
        LOG_INFO("Press F1 to toggle camera debugging.");
        LOG_INFO("Press F2 to log camera properties.");
        LOG_INFO("Press F3 to start/stop camera position recording.");
        LOG_INFO("Press F4 to analyze logged camera data.");
    }
    
    // Set up running state
    m_Running = true;
    
    LOG_INFO("Sylva Engine initialized successfully.");
    return true;
}

void Core::Run() {
    LOG_INFO("Starting main game loop...");
    
    float lastFrameTime = m_Platform.GetTime();
    
    // Main game loop
    while (m_Running) {
        // Calculate delta time
        float currentTime = m_Platform.GetTime();
        m_DeltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        
        // Process window events and input
        m_Platform.PollEvents();
        
        // Process debug input commands
        ProcessDebugInput();
        
        // Check if window should close
        if (m_Platform.WindowShouldClose()) {
            m_Running = false;
            continue;
        }
        
        // Get camera for the frame (now using reference)
        Camera& camera = m_CameraController ? m_CameraController->GetCamera() : *m_Renderer.GetCamera();
        
        // Update camera debug system
        if (m_CameraDebug && m_CameraDebug->IsEnabled()) {
            m_CameraDebug->Update(m_DeltaTime);
        }
        
        // Update world (which updates the player)
        m_World.Update(m_DeltaTime, &camera);
        
        // Update camera controller (player position is now handled inside the controller)
        if (m_CameraController) {
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
    LOG_INFO("Shutting down Sylva Engine...");
    
    // Shutdown in reverse order of initialization
    m_CameraDebug.reset();
    m_CameraController.reset(); // Ensure controller is destroyed before renderer
    m_Renderer.Shutdown();
    m_Platform.Shutdown();
}

void Core::EnableCameraDebugging(bool enabled) {
    if (m_CameraDebug) {
        m_CameraDebug->SetEnabled(enabled);
        LOG_INFO("Camera debugging ", enabled ? "enabled" : "disabled");
    }
}

bool Core::IsCameraDebuggingEnabled() const {
    return m_CameraDebug && m_CameraDebug->IsEnabled();
}

void Core::ProcessDebugInput() {
    // Process debug key presses (function keys)
    
    // F1: Toggle camera debugging
    static bool f1Pressed = false;
    bool isF1Down = m_Platform.IsKeyPressed(GLFW_KEY_F1);
    if (isF1Down && !f1Pressed) {
        bool newState = !IsCameraDebuggingEnabled();
        EnableCameraDebugging(newState);
    }
    f1Pressed = isF1Down;
    
    // F2: Log camera properties
    static bool f2Pressed = false;
    bool isF2Down = m_Platform.IsKeyPressed(GLFW_KEY_F2);
    if (isF2Down && !f2Pressed) {
        if (m_CameraDebug) {
            m_CameraDebug->LogCameraProperties();
        }
    }
    f2Pressed = isF2Down;
    
    // F3: Start/stop camera position recording
    static bool f3Pressed = false;
    bool isF3Down = m_Platform.IsKeyPressed(GLFW_KEY_F3);
    if (isF3Down && !f3Pressed) {
        if (m_CameraDebug) {
            static bool isRecording = false;
            if (isRecording) {
                m_CameraDebug->StopRecording();
            } else {
                m_CameraDebug->StartRecording();
            }
            isRecording = !isRecording;
        }
    }
    f3Pressed = isF3Down;
    
    // F4: Analyze logged camera data
    static bool f4Pressed = false;
    bool isF4Down = m_Platform.IsKeyPressed(GLFW_KEY_F4);
    if (isF4Down && !f4Pressed) {
        if (m_CameraDebug) {
            m_CameraDebug->AnalyzeLoggedData();
        }
    }
    f4Pressed = isF4Down;
    
    // Additional camera mode toggle (existing functionality)
    static bool cPressed = false;
    bool isCDown = m_Platform.IsKeyPressed(GLFW_KEY_C);
    if (isCDown && !cPressed) {
        if (m_CameraController) {
            auto mode = m_CameraController->GetCameraMode();
            if (mode == CameraController::CameraMode::ThirdPersonOrbit) {
                m_CameraController->SetCameraMode(CameraController::CameraMode::FixedFollow);
                LOG_INFO("Camera mode: Fixed Follow (Cube World style)");
            } else if (mode == CameraController::CameraMode::FixedFollow) {
                m_CameraController->SetCameraMode(CameraController::CameraMode::Free);
                LOG_INFO("Camera mode: Free Camera");
            } else {
                m_CameraController->SetCameraMode(CameraController::CameraMode::ThirdPersonOrbit);
                LOG_INFO("Camera mode: Third-Person Orbit Camera");
            }
        }
    }
    cPressed = isCDown;
}

} // namespace Sylva 
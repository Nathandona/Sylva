#pragma once

#include "../platform/Platform.h"
#include "../renderer/Renderer.h"
#include "../renderer/Camera.h"
#include "../renderer/CameraController.h"
#include "../world/World.h"
#include "../input/InputManager.h"
#include <memory>

namespace Sylva {

class Core {
public:
    Core();
    ~Core();

    // Initialize the engine (window, OpenGL, etc.)
    bool Initialize();
    
    // Run the main game loop
    void Run();
    
    // Clean up resources and exit
    void Shutdown();

private:
    // Main game loop related
    bool m_Running;
    float m_DeltaTime;
    
    // Core systems
    Platform m_Platform;
    Renderer m_Renderer;
    World m_World;
    InputManager m_InputManager;
    
    // Camera controller (owned by Core)
    std::unique_ptr<CameraController> m_CameraController;
};

} 
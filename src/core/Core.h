#pragma once

#include "../platform/Platform.h"
#include "../renderer/Renderer.h"

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
};

} 
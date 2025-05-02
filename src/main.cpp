#include "core/Core.h"

int main(int argc, char* argv[]) {
    // Create and initialize the core engine
    Sylva::Core core;
    
    // Initialize the engine (creates window, initializes OpenGL, etc.)
    if (!core.Initialize()) {
        return 1;
    }
    
    // Run the main game loop
    core.Run();
    
    // Clean up and exit
    core.Shutdown();
    
    return 0;
} 
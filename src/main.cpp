#include "core/Core.h"
#include "core/Config.h"
#include "core/Logger.h"

// Forward declarations of initialization functions
void InitializeConfig(int argc, char* argv[]);
void InitializeLogger();

int main(int argc, char* argv[]) {
    // Initialize configuration system first
    InitializeConfig(argc, argv);
    
    // Initialize logging system
    InitializeLogger();
    
    // Log startup information
    LOG_INFO("Sylva Engine starting up...");
    
    // Create and initialize the core engine
    Sylva::Core core;
    
    // Initialize the engine (creates window, initializes OpenGL, etc.)
    if (!core.Initialize()) {
        LOG_FATAL("Failed to initialize engine, exiting");
        return 1;
    }
    
    LOG_INFO("Engine initialized successfully");
    
    // Run the main game loop
    LOG_INFO("Starting main loop");
    core.Run();
    
    // Clean up and exit
    LOG_INFO("Shutting down engine");
    core.Shutdown();
    
    LOG_INFO("Sylva Engine shut down successfully");
    
    return 0;
} 
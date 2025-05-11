#include "core/logger.h"
#include "core/config.h"
#include "core/types.h"
#include "platform/window.h"
#include "platform/input.h"
#include "renderer/camera.h"
#include "renderer/ui.h"
#include "renderer/shader.h"
#include "world/player.h"
#include "world/voxel_world.h"
#include "audio/audio_system.h"
#include "engine.h"
#include "system_manager.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// OpenGL includes
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Forward declarations of systems we'll implement later
namespace Sylva {
    class Camera;
    class Player;
    class VoxelWorld;
    namespace UI {
        void renderCrosshair(const Camera& camera);
    }
}

int main() {
    using namespace Sylva;
    if (!SystemManager::initializeSystems()) {
        std::cerr << "System initialization failed!" << std::endl;
        return 1;
    }
    Engine engine;
    if (!engine.initialize()) {
        std::cerr << "Engine initialization failed!" << std::endl;
        SystemManager::shutdownSystems();
        return 1;
    }
    engine.run();
    engine.shutdown();
    SystemManager::shutdownSystems();
    return 0;
} 
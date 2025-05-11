#pragma once

#include <memory>
#include <string>

namespace Sylva {

class Engine {
public:
    Engine();
    ~Engine();
    bool initialize(const std::string& configPath = "config/default_config.ini");
    void run();
    void shutdown();
private:
    // Add pointers to subsystems as needed
    class Window* m_window;
    class Player* m_player;
    class Camera* m_camera;
    class VoxelWorld* m_voxelWorld;
    class Shader* m_playerShader;
    unsigned int m_musicId = 0;
    // ... other subsystems
};

} // namespace Sylva 
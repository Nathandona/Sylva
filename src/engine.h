#pragma once

#include <memory>
#include <string>

namespace Sylva {

class Window;
class Player;
class Camera;
class VoxelWorld;
class Shader;

class Engine {
public:
    Engine();
    ~Engine();
    bool initialize(const std::string& configPath = "config/default_config.ini");
    void run();
    void shutdown();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Player> m_player;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<VoxelWorld> m_voxelWorld;
    std::unique_ptr<Shader> m_playerShader;
    unsigned int m_musicId = 0;
    bool m_shutdownComplete = false;
};

} // namespace Sylva

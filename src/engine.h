#pragma once

#include <memory>
#include <string>

namespace Sylva {

class Window;
class Player;
class Camera;
class VoxelWorld;
class Shader;
class IAudioSystem;
class UISystem;

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
    void tick(float deltaTime);
    void renderFrame(float aspectRatio);
    void handleDebugToggles();
    void logFrameStats(float deltaTime);

    std::unique_ptr<Window> m_window;
    std::unique_ptr<Player> m_player;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<VoxelWorld> m_voxelWorld;
    std::unique_ptr<Shader> m_playerShader;
    std::unique_ptr<IAudioSystem> m_audio;
    std::unique_ptr<UISystem> m_ui;
    unsigned int m_musicId = 0;
    bool m_shutdownComplete = false;

    // Debug-toggle edge tracking (F1).
    bool m_f1KeyDown = false;

    // FPS smoothing: accumulate over N frames before logging an average.
    float m_fpsAccum = 0.0f;
    int   m_fpsFrames = 0;
};

} // namespace Sylva

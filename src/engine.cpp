#include "engine.h"
#include "core/logger.h"
#include "core/config.h"
#include "platform/window.h"
#include "platform/input.h"
#include "renderer/camera.h"
#include "renderer/ui.h"
#include "renderer/shader.h"
#include "world/player.h"
#include "world/voxel_world.h"
#include "audio/audio_system.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <exception>

namespace Sylva {

Engine::Engine() = default;

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize(const std::string& configPath) {
    // Config: default required, user overrides optional.
    if (!Config::load(configPath)) {
        Logger::logError("Failed to load default configuration!");
        return false;
    }
    Config::load("config/user_config.ini");

    // Logger: level + optional file output, both config-driven.
    const std::string levelStr = Config::getString("Logging.level");
    LogLevel level = LogLevel::INFO;
    if (levelStr == "DEBUG") {
        level = LogLevel::DEBUG;
    } else if (levelStr == "WARNING") {
        level = LogLevel::WARNING;
    } else if (levelStr == "ERROR") {
        level = LogLevel::ERROR;
    }
    Logger::setLogLevel(level);
    const std::string logFile = Config::getString("Logging.file");
    if (!logFile.empty()) {
        Logger::setLogFile(logFile);
    }
    Logger::logInfo("Sylva Engine starting...");
    Logger::logInfo("Version: 0.1.0");

    // GLFW (one-shot init for the process).
    if (!Window::initialize()) {
        Logger::logError("Failed to initialize GLFW!");
        return false;
    }

    m_window = std::make_unique<Window>();
    if (!m_window->create(Config::getInt("Window.width"),
                          Config::getInt("Window.height"),
                          Config::getString("Window.title"),
                          Config::getBool("Window.fullscreen"))) {
        Logger::logError("Failed to create window!");
        return false;
    }
    m_window->setVSync(Config::getBool("Graphics.vsync", true));
    // Initialize Input system
    Input::initialize(m_window->getGLFWwindow());
    // Initialize UI overlay
    m_ui = std::make_unique<UISystem>(m_window->getWidth(), m_window->getHeight());
    // Set up window resize callback — forwards to the engine-owned UI instance.
    m_window->setResizeCallback([this](int width, int height) {
        if (m_ui)
            m_ui->resize(width, height);
        Logger::logInfo("Window resized to " + std::to_string(width) + "x" + std::to_string(height));
    });
    // Audio: ctor opens the AL device + reads volumes/mute states from
    // config via the per-type metadata table.
    m_audio = std::make_unique<OpenALAudioSystem>();
    if (!m_audio->isReady()) {
        Logger::logError("Failed to initialize audio system");
        return false;
    }
    m_camera = std::make_unique<Camera>();
    m_player = std::make_unique<Player>();
    m_voxelWorld = std::make_unique<VoxelWorld>();
    if (!m_voxelWorld->initializeGraphics()) {
        Logger::logError("Failed to initialize voxel world graphics");
        return false;
    }
    // TODO: Replace with ShaderManager later
    try {
        m_playerShader = std::make_unique<Shader>("assets/shaders/colored.vert", "assets/shaders/colored.frag");
    } catch (const std::exception& e) {
        Logger::logError("Failed to load player shader: " + std::string(e.what()));
        return false;
    }
    // Enable collision debug visualization if specified in config
    bool const collisionDebug = Config::getBool("Debug.collision_visualization", false);
    m_voxelWorld->setCollisionDebugEnabled(collisionDebug);
    Logger::logInfo("Collision debug visualization: " + std::string(collisionDebug ? "enabled" : "disabled"));
    // Player visual params (mesh + shader live elsewhere now).
    m_player->setSize(1.0f, 1.0f);
    m_player->setColor({1.0f, 1.0f, 1.0f, 1.0f});
    // Set up camera
    m_camera->setTargetHeight(1.7f);
    m_camera->setDistance(5.0f);
    // Generate the voxel world around the player's starting position
    m_voxelWorld->generateWorld(m_player->getPosition());
    // Audio asset paths come from [AudioAssets] in config so users can swap
    // files (or point at WAVs) without touching code.
    const std::string startupPath = Config::getString("AudioAssets.startup_sfx", "assets/audio/sfx/startup.wav");
    const std::string footstepPath = Config::getString("AudioAssets.footstep_sfx", "assets/audio/sfx/footstep.wav");
    const std::string musicPath = Config::getString("AudioAssets.music_track", "");
    m_audio->loadSound("startup", startupPath, AudioType::SOUND_EFFECT);
    m_audio->loadSound("footstep", footstepPath, AudioType::SOUND_EFFECT);
    if (!musicPath.empty()) {
        if (m_audio->loadSound("background", musicPath, AudioType::MUSIC)) {
            m_musicId = m_audio->playSound("background", true, 0.7f);
        }
    }
    m_audio->playSound("startup");
    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return true;
}

void Engine::run() {
    Logger::logInfo("Entering main loop...");
    const int maxFrames = Config::getInt("Debug.max_frames", 0);
    int frames = 0;
    while (!m_window->shouldClose() && (maxFrames == 0 || frames < maxFrames)) {
        const float deltaTime = m_window->getDeltaTime();
        m_window->pollEvents();
        tick(deltaTime);
        const float aspectRatio = static_cast<float>(m_window->getWidth()) / static_cast<float>(m_window->getHeight());
        renderFrame(aspectRatio);
        m_window->swapBuffers();
        ++frames;
        logFrameStats(deltaTime);
    }
}

void Engine::tick(float deltaTime) {
    const InputState& inputState = Input::getState();
    m_player->updateMovement(deltaTime, inputState, m_camera->getForward(), m_camera->getRight(), *m_voxelWorld);
    m_camera->updateOrbit(deltaTime, *m_player, inputState, *m_voxelWorld);
    m_voxelWorld->update(deltaTime, m_player->getPosition());
    handleDebugToggles();
    Input::update();
    m_audio->update();
    const glm::vec3 camPos = m_camera->getPosition();
    const glm::vec3 camFwd = m_camera->getForward();
    const glm::vec3 camUp = m_camera->getUp();
    m_audio->setListenerPosition(glm::value_ptr(camPos));
    m_audio->setListenerOrientation(glm::value_ptr(camFwd), glm::value_ptr(camUp));
}

void Engine::renderFrame(float aspectRatio) {
    // Clear color matches voxel-shader fog so chunks fade into the sky at
    // the view-distance edge instead of cutting off against the background.
    glClearColor(Config::getFloat("Fog.color.x", 0.5f),
                 Config::getFloat("Fog.color.y", 0.7f),
                 Config::getFloat("Fog.color.z", 0.9f),
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const glm::mat4 view = m_camera->getViewMatrix();
    const glm::mat4 proj = m_camera->getProjectionMatrix(aspectRatio);
    m_voxelWorld->render(*m_camera, aspectRatio);
    if (m_voxelWorld->isCollisionDebugEnabled()) {
        m_voxelWorld->renderCollisionDebug(*m_camera, *m_player, aspectRatio);
    }
    m_player->renderPlayer(*m_playerShader, view, proj);
    if (m_ui)
        m_ui->renderCrosshair();
}

void Engine::handleDebugToggles() {
    // F1: rising-edge toggle for collision debug visualization.
    const bool f1Pressed = Input::isKeyPressed(GLFW_KEY_F1);
    if (f1Pressed && !m_f1KeyDown) {
        const bool enabled = !m_voxelWorld->isCollisionDebugEnabled();
        m_voxelWorld->setCollisionDebugEnabled(enabled);
        Logger::logInfo(std::string("Collision debug visualization: ") + (enabled ? "enabled" : "disabled"));
    }
    m_f1KeyDown = f1Pressed;
}

void Engine::logFrameStats(float deltaTime) {
    // Smooth FPS over 60 frames instead of logging single-frame instantaneous.
    m_fpsAccum += deltaTime;
    if (++m_fpsFrames >= 60) {
        const float avgFps = m_fpsFrames / m_fpsAccum;
        Logger::logDebug("FPS (avg over " + std::to_string(m_fpsFrames) + " frames): " + std::to_string(avgFps));
        const glm::vec3 p = m_player->getPosition();
        Logger::logDebug("Player position: " + std::to_string(p.x) + ", " + std::to_string(p.y) + ", " +
                         std::to_string(p.z));
        m_fpsAccum = 0.0f;
        m_fpsFrames = 0;
    }
}

void Engine::shutdown() {
    if (m_shutdownComplete)
        return;
    Logger::logInfo("Cleaning up...");
    if (m_musicId != 0 && m_audio) {
        m_audio->stopSound(m_musicId);
        m_musicId = 0;
    }
    // Order matters: every GL-owning object must die before the window
    // tears down the GL context.
    m_ui.reset();
    m_playerShader.reset();
    m_voxelWorld.reset();
    m_camera.reset();
    m_player.reset();
    if (m_window) {
        m_window->shutdown();
        m_window.reset();
    }
    // Clear input module statics — GLFW callbacks have been unregistered with
    // the window above; this resets the captured state struct so a second
    // Engine instance / test run starts clean.
    Input::shutdown();
    // Audio is context-independent; tear it down last.
    m_audio.reset();
    m_shutdownComplete = true;
    Logger::logInfo("Sylva Engine shut down.");
}

} // namespace Sylva
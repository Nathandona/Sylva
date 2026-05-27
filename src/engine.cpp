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
#include <string>
#include <thread>
#include <chrono>
#include <iostream>

namespace Sylva {

Engine::Engine() = default;

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize(const std::string& configPath) {
    m_window = std::make_unique<Window>();
    if (!m_window->create(
        Config::getInt("Window.width"),
        Config::getInt("Window.height"),
        Config::getString("Window.title"),
        Config::getBool("Window.fullscreen")
    )) {
        Logger::logError("Failed to create window!");
        return false;
    }
    m_window->setVSync(Config::getBool("Graphics.vsync", true));
    // Initialize Input system
    Input::initialize(m_window->getGLFWwindow());
    // Initialize UI
    UI::initialize(m_window->getWidth(), m_window->getHeight());
    // Set up window resize callback
    m_window->setResizeCallback([](int width, int height) {
        UI::resize(width, height);
        Logger::logInfo("Window resized to " + std::to_string(width) + "x" + std::to_string(height));
    });
    // Initialize audio system
    if (!AudioSystem::initialize()) {
        Logger::logError("Failed to initialize audio system");
        return false;
    }
    // Set audio volumes from config
    AudioSystem::setMasterVolume(Config::getFloat("Audio.master_volume", 0.8f));
    AudioSystem::setTypeVolume(AudioType::SOUND_EFFECT, Config::getFloat("Audio.sound_effect_volume", 1.0f));
    AudioSystem::setTypeVolume(AudioType::MUSIC, Config::getFloat("Audio.music_volume", 0.7f));
    AudioSystem::setTypeVolume(AudioType::AMBIENT, Config::getFloat("Audio.ambient_volume", 0.5f));
    AudioSystem::setTypeVolume(AudioType::VOICE, Config::getFloat("Audio.voice_volume", 1.0f));
    // Initialize mute states from config
    if (Config::getBool("Audio.mute_music", false)) {
        AudioSystem::setTypeMuted(AudioType::MUSIC, true);
    }
    if (Config::getBool("Audio.mute_sfx", false)) {
        AudioSystem::setTypeMuted(AudioType::SOUND_EFFECT, true);
    }
    if (Config::getBool("Audio.mute_ambient", false)) {
        AudioSystem::setTypeMuted(AudioType::AMBIENT, true);
    }
    if (Config::getBool("Audio.mute_voice", false)) {
        AudioSystem::setTypeMuted(AudioType::VOICE, true);
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
    bool collisionDebug = Config::getBool("Debug.collision_visualization", false);
    m_voxelWorld->setCollisionDebugEnabled(collisionDebug);
    Logger::logInfo("Collision debug visualization: " + std::string(collisionDebug ? "enabled" : "disabled"));
    // Initialize player graphics
    if (!m_player->initializeGraphics()) {
        Logger::logError("Failed to initialize player graphics");
        return false;
    }
    // Initialize player parameters
    m_player->setSize(1.0f, 1.0f);
    m_player->setColor({1.0f, 1.0f, 1.0f, 1.0f});
    // Set up camera
    m_camera->setTargetHeight(1.7f);
    m_camera->setDistance(5.0f);
    // Generate the voxel world around the player's starting position
    m_voxelWorld->generateWorld(m_player->getPosition());
    // Load some example audio
    AudioSystem::loadSound("startup", "assets/audio/sfx/startup.wav", AudioType::SOUND_EFFECT);
    AudioSystem::loadSound("background", "assets/audio/music/background.ogg", AudioType::MUSIC);
    AudioSystem::loadSound("footstep", "assets/audio/sfx/footstep.wav", AudioType::SOUND_EFFECT);
    // Play startup sound
    AudioSystem::playSound("startup");
    // Start background music (looped)
    m_musicId = AudioSystem::playSound("background", true, 0.7f);
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
    int maxFrames = Config::getInt("Debug.max_frames", 0);
    int frames = 0;
    bool f1KeyDown = false;
    while (!m_window->shouldClose() && (maxFrames == 0 || frames < maxFrames)) {
        float deltaTime = m_window->getDeltaTime();
        m_window->pollEvents();
        const InputState& inputState = Input::getState();
        m_player->updateMovement(deltaTime, inputState, *m_voxelWorld, *m_camera);
        m_camera->updateOrbit(deltaTime, *m_player, inputState);
        m_voxelWorld->update(deltaTime, m_player->getPosition());
        bool f1KeyPressed = Input::isKeyPressed(GLFW_KEY_F1);
        if (f1KeyPressed && !f1KeyDown) {
            bool collisionDebug = !m_voxelWorld->isCollisionDebugEnabled();
            m_voxelWorld->setCollisionDebugEnabled(collisionDebug);
            Logger::logInfo("Collision debug visualization: " + std::string(collisionDebug ? "enabled" : "disabled"));
        }
        f1KeyDown = f1KeyPressed;
        Input::update();
        AudioSystem::update();
        float cameraPos[3] = {
            m_camera->getPosition().x,
            m_camera->getPosition().y,
            m_camera->getPosition().z
        };
        float cameraForward[3] = {
            m_camera->getForward().x,
            m_camera->getForward().y,
            m_camera->getForward().z
        };
        float cameraUp[3] = {
            m_camera->getUp().x,
            m_camera->getUp().y,
            m_camera->getUp().z
        };
        AudioSystem::setListenerPosition(cameraPos);
        AudioSystem::setListenerOrientation(cameraForward, cameraUp);
        if (frames % 30 == 0) {
            AudioSystem::playSound("footstep", false, 0.5f);
        }
        glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float aspectRatio = static_cast<float>(m_window->getWidth()) / static_cast<float>(m_window->getHeight());
        glm::mat4 projectionMatrix = m_camera->getProjectionMatrix(aspectRatio);
        glm::mat4 viewMatrix = m_camera->getViewMatrix();
        m_voxelWorld->render(*m_camera);
        if (m_voxelWorld->isCollisionDebugEnabled()) {
            m_voxelWorld->renderCollisionDebug(*m_camera, *m_player);
        }
        m_player->renderPlayer(*m_playerShader, viewMatrix, projectionMatrix);
        UI::renderCrosshair(*m_camera);
        m_window->swapBuffers();
        frames++;
        if (frames % 60 == 0) {
            Logger::logDebug("FPS: " + std::to_string(1.0f / deltaTime));
            Logger::logDebug("Player position: " +
                            std::to_string(m_player->getPosition().x) + ", " +
                            std::to_string(m_player->getPosition().y) + ", " +
                            std::to_string(m_player->getPosition().z));
        }
    }
}

void Engine::shutdown() {
    if (!m_window && !m_player && !m_camera && !m_voxelWorld && !m_playerShader) {
        return; // already shut down (idempotent)
    }
    Logger::logInfo("Cleaning up...");
    if (m_musicId != 0) {
        AudioSystem::stopSound(m_musicId);
        m_musicId = 0;
    }
    // Release GL resources held by free systems before the window/context dies.
    UI::shutdown();
    // Order matters: shaders + GL-owning objects before the window destroys the GL context.
    m_playerShader.reset();
    m_voxelWorld.reset();
    m_camera.reset();
    m_player.reset();
    if (m_window) {
        m_window->shutdown();
        m_window.reset();
    }
    Logger::logInfo("Sylva Engine shut down.");
}

} // namespace Sylva 
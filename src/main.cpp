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

    // Initialize configuration
    if (!Config::load("config/default_config.ini")) {
        std::cerr << "Failed to load default configuration!" << std::endl;
        return 1;
    }
    
    // Load user config (overrides defaults)
    Config::load("config/user_config.ini");
    
    // Initialize logger
    Logger::setLogLevel(
        Config::getString("Logging.level") == "DEBUG" ? LogLevel::DEBUG :
        Config::getString("Logging.level") == "WARNING" ? LogLevel::WARNING :
        Config::getString("Logging.level") == "ERROR" ? LogLevel::ERROR :
        LogLevel::INFO
    );
    
    std::string logFile = Config::getString("Logging.file");
    if (!logFile.empty()) {
        Logger::setLogFile(logFile);
    }
    
    Logger::logInfo("Sylva Engine starting...");
    Logger::logInfo("Version: 0.1.0");
    
    // Log configuration
    Logger::logInfo("Window size: " + 
        std::to_string(Config::getInt("Window.width")) + "x" + 
        std::to_string(Config::getInt("Window.height")));
    Logger::logInfo("Fullscreen: " + 
        std::string(Config::getBool("Window.fullscreen") ? "Yes" : "No"));
    
    // Initialize systems
    Logger::logInfo("Initializing systems...");
    
    // Initialize GLFW and Window
    if (!Window::initialize()) {
        Logger::logError("Failed to initialize GLFW!");
        return 1;
    }
    
    // Create window
    Window window;
    if (!window.create(
        Config::getInt("Window.width"), 
        Config::getInt("Window.height"),
        Config::getString("Window.title"),
        Config::getBool("Window.fullscreen")
    )) {
        Logger::logError("Failed to create window!");
        return 1;
    }
    
    // Set VSync
    window.setVSync(Config::getBool("Graphics.vsync", true));
    
    // Initialize Input system
    Input::initialize(window.getGLFWwindow());
    
    // Initialize UI
    UI::initialize(window.getWidth(), window.getHeight());
    
    // Set up window resize callback
    window.setResizeCallback([](int width, int height) {
        // Update UI when window is resized
        UI::resize(width, height);
        Logger::logInfo("Window resized to " + std::to_string(width) + "x" + std::to_string(height));
    });
    
    // Initialize audio system
    if (!AudioSystem::initialize()) {
        Logger::logError("Failed to initialize audio system");
        return 1;
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
    
    // Initialize camera, player, voxel world
    Camera camera;
    Player player;
    VoxelWorld voxelWorld;
    
    // Initialize voxel world graphics
    if (!voxelWorld.initializeGraphics()) {
        Logger::logError("Failed to initialize voxel world graphics");
        return 1;
    }
    
    // TODO: Replace with ShaderManager later
    Shader playerShader("assets/shaders/colored.vert", "assets/shaders/colored.frag");
    
    // Enable collision debug visualization if specified in config
    bool collisionDebug = Config::getBool("Debug.collision_visualization", false);
    voxelWorld.setCollisionDebugEnabled(collisionDebug);
    Logger::logInfo("Collision debug visualization: " + std::string(collisionDebug ? "enabled" : "disabled"));
    
    // Initialize player graphics
    if (!player.initializeGraphics()) {
        Logger::logError("Failed to initialize player graphics");
        return 1;
    }
    
    // Initialize player parameters
    player.setSize(1.0f, 1.0f);
    player.setColor({1.0f, 1.0f, 1.0f, 1.0f});
    
    // Set up camera
    camera.setTargetHeight(1.7f);  // Eye level
    camera.setDistance(5.0f);      // Default distance
    
    // Generate the voxel world around the player's starting position
    voxelWorld.generateWorld(player.getPosition());
    
    // Load some example audio
    AudioSystem::loadSound("startup", "assets/audio/sfx/startup.wav", AudioType::SOUND_EFFECT);
    AudioSystem::loadSound("background", "assets/audio/music/background.ogg", AudioType::MUSIC);
    AudioSystem::loadSound("footstep", "assets/audio/sfx/footstep.wav", AudioType::SOUND_EFFECT);
    
    // Play startup sound
    AudioSystem::playSound("startup");
    
    // Start background music (looped)
    unsigned int musicId = AudioSystem::playSound("background", true, 0.7f);
    
    // OpenGL settings
    glEnable(GL_DEPTH_TEST);  // Enable depth testing
    glEnable(GL_CULL_FACE);   // Enable face culling
    glCullFace(GL_BACK);      // Cull back faces
    
    // Enable alpha blending for transparent blocks
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    Logger::logInfo("Entering main loop...");
    
    // For demo purposes, limit frames
    int maxFrames = Config::getInt("Debug.max_frames", 0);
    int frames = 0;
    
    // Key state tracking
    bool f1KeyDown = false;
    
    // Main loop
    while (!window.shouldClose() && (maxFrames == 0 || frames < maxFrames)) {
        // Get delta time for smooth animations
        float deltaTime = window.getDeltaTime();
        
        // Process events
        window.pollEvents();
        
        // Get input state before updating any systems
        const InputState& inputState = Input::getState();
        
        // Update systems with the current input state
        player.updateMovement(deltaTime, inputState, voxelWorld, camera);
        camera.updateOrbit(deltaTime, player, inputState);
        
        // Update the voxel world
        voxelWorld.update(deltaTime, player.getPosition());
        
        // Toggle collision debug visualization when F1 is pressed (once per press)
        bool f1KeyPressed = Input::isKeyPressed(GLFW_KEY_F1);
        if (f1KeyPressed && !f1KeyDown) {
            collisionDebug = !collisionDebug;
            voxelWorld.setCollisionDebugEnabled(collisionDebug);
            Logger::logInfo("Collision debug visualization: " + std::string(collisionDebug ? "enabled" : "disabled"));
        }
        f1KeyDown = f1KeyPressed;
        
        // Update input (resets deltas for next frame) - moved after camera update
        Input::update();
        
        // Update audio system
        AudioSystem::update();
        
        // Update audio listener position to match camera
        float cameraPos[3] = { 
            camera.getPosition().x, 
            camera.getPosition().y, 
            camera.getPosition().z 
        };
        float cameraForward[3] = { 
            camera.getForward().x, 
            camera.getForward().y, 
            camera.getForward().z 
        };
        float cameraUp[3] = { 
            camera.getUp().x,
            camera.getUp().y,
            camera.getUp().z
        };
        
        AudioSystem::setListenerPosition(cameraPos);
        AudioSystem::setListenerOrientation(cameraForward, cameraUp);
        
        // Play footstep sound occasionally (every 30 frames in this example)
        if (frames % 30 == 0) {
            AudioSystem::playSound("footstep", false, 0.5f);
        }
        
        // Clear the screen
        glClearColor(0.5f, 0.7f, 0.9f, 1.0f); // Light blue background for sky
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Calculate projection matrix (same for all objects)
        float aspectRatio = static_cast<float>(window.getWidth()) / static_cast<float>(window.getHeight());
        glm::mat4 projectionMatrix = camera.getProjectionMatrix(aspectRatio);
        glm::mat4 viewMatrix = camera.getViewMatrix();
        
        // Render voxel world
        voxelWorld.render(camera);
        
        // Render collision debug if enabled
        if (voxelWorld.isCollisionDebugEnabled()) {
            voxelWorld.renderCollisionDebug(camera, player);
        }
        
        // Render player
        player.renderPlayer(playerShader, viewMatrix, projectionMatrix);
        
        // Render UI
        UI::renderCrosshair(camera);
        
        // Swap buffers
        window.swapBuffers();
        
        frames++;
        
        // For testing: log every 60 frames
        if (frames % 60 == 0) {
            Logger::logDebug("FPS: " + std::to_string(1.0f / deltaTime));
            Logger::logDebug("Player position: " + 
                            std::to_string(player.getPosition().x) + ", " +
                            std::to_string(player.getPosition().y) + ", " +
                            std::to_string(player.getPosition().z));
        }
    }
    
    // Cleanup
    Logger::logInfo("Cleaning up...");
    
    // Stop audio
    AudioSystem::stopSound(musicId);
    AudioSystem::shutdown();
    
    // Shutdown window
    window.shutdown();
    
    Logger::logInfo("Sylva Engine shut down.");
    return 0;
} 
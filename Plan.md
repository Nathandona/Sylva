# Sylva Engine: Restart & Modular System Implementation Guide

This document provides a step-by-step process for restarting the Sylva project from scratch, focusing on a modular, maintainable architecture. It covers the implementation of core systems: Camera, Player, World, UI, Logger, and Config, and suggests future improvements.

---

## 1. Project Initialization

### 1.1. Clean Start ✅ COMPLETED

- **Backup** any important code or assets.
- **Delete** or archive the existing `src/`, `build/`, and configuration files as needed.
- **Re-initialize** the repository (optional):  
  ```sh
  git init
  ```

### 1.2. Directory Structure ✅ COMPLETED

```
Sylva/
├── assets/
│   ├── shaders/
│   │   ├── basic.vert
│   │   └── basic.frag
│   ├── textures/
│   │   └── default.png
│   ├── models/
│   │   └── player.obj
├── config/
│   ├── default_config.ini    # Engine defaults
│   └── user_config.ini       # User overrides
├── src/
│   ├── core/
│   │   ├── logger.h/.cpp     # Logging system
│   │   ├── config.h/.cpp     # Configuration management
│   │   └── types.h           # Common type definitions
│   ├── platform/
│   │   ├── window.h/.cpp     # Window management
│   │   └── input.h/.cpp      # Input handling
│   ├── renderer/
│   │   ├── camera.h/.cpp     # Camera system
│   │   ├── shader.h/.cpp     # Shader management
│   │   └── ui.h/.cpp         # UI rendering
│   ├── world/
│   │   ├── player.h/.cpp     # Player system
│   │   ├── terrain.h/.cpp    # Terrain generation
│   │   └── collision.h/.cpp  # Collision detection
│   └── main.cpp              # Application entry point
├── vendor/                   # Third-party libraries
├── CMakeLists.txt           # Build configuration
└── vcpkg.json               # Dependency management
```

---

## 2. Dependency Setup ✅ COMPLETED

- Use **vcpkg** for all dependencies.
- Update `vcpkg.json` and document each dependency's purpose.
- Integrate dependencies in `CMakeLists.txt` using `find_package()` and `target_link_libraries()`.

**Core dependencies:**
- GLFW3 (window/input) - Version 3.3.8 or later
- GLAD (OpenGL loader) - Latest version
- GLM (math) - Version 0.9.9.8 or later
- tinyobjloader (model loading) - Version 2.0.0 or later

**Example vcpkg.json:**
```json
{
  "name": "sylva",
  "version": "0.1.0",
  "dependencies": [
    "glfw3",
    "glad",
    "glm",
    "tinyobjloader"
  ]
}
```

---

## 3. System Implementations

### 3.1. Logger System (`src/core/logger.h/.cpp`) ✅ COMPLETED

**Features:**
- Centralized logging utility for the engine.
- Supports log levels (info, warning, error, debug).
- Outputs to console and optionally to a file.
- Configurable via config file.

**Log Levels:**
```cpp
enum class LogLevel {
    DEBUG,   // Detailed information for debugging
    INFO,    // General information about program execution
    WARNING, // Potentially harmful situations
    ERROR    // Error events that might still allow the program to continue
};
```

**Modular Functions:**
- `Logger::logInfo(const std::string&)` - Log general information
- `Logger::logWarning(const std::string&)` - Log potential issues
- `Logger::logError(const std::string&)` - Log errors
- `Logger::logDebug(const std::string&)` - Log debug information
- `Logger::setLogLevel(LogLevel)` - Set minimum log level
- `Logger::setLogFile(const std::string&)` - Set log file path

**Example Usage:**
```cpp
Logger::setLogLevel(LogLevel::DEBUG);
Logger::logInfo("Engine initialized");
Logger::logDebug("Camera position: " + std::to_string(camera.getPosition().x));
```

---

### 3.2. Config System (`src/core/config.h/.cpp`, `config/`) ✅ COMPLETED

**Features:**
- Loads engine and game settings from `.ini` or `.json` files.
- Supports default and user-specific overrides.
- Provides access to configuration values throughout the engine.

**Example Config File (`config/default_config.ini`):**
```ini
[Window]
width = 1280
height = 720
title = "Sylva Engine"
fullscreen = false

[Graphics]
vsync = true
msaa = 4
shadow_quality = high

[Input]
mouse_sensitivity = 0.5
invert_y = false

[Logging]
level = INFO
file = "logs/engine.log"
```

**Modular Functions:**
- `Config::load(const std::string& path)` - Load config file
- `Config::getString(const std::string& key)` - Get string value
- `Config::getInt(const std::string& key)` - Get integer value
- `Config::getFloat(const std::string& key)` - Get float value
- `Config::getBool(const std::string& key)` - Get boolean value
- `Config::set(const std::string& key, const ValueType& value)` - Set value

**Example Usage:**
```cpp
int windowWidth = Config::getInt("Window.width");
float mouseSensitivity = Config::getFloat("Input.mouse_sensitivity");
```

---

### 3.3. Window System (`src/platform/window.h/.cpp`) ✅ COMPLETED

**Features:**
- Abstraction over GLFW for window creation and management.
- Handles window initialization and termination.
- Creates the main application window based on configuration settings.
- Sets up OpenGL context with GLAD loader.
- Manages window events and the main loop condition.
- Provides access to the underlying GLFW window handle if needed.
- Sets up basic GLFW error handling.
- Handles window resizing and viewport updates.
- Calculates frame delta time for smooth animations.
- Supports VSync configuration.
- Implements callbacks for window events.

**Window Parameters (Loaded from Config):**
- `Window.width`: Initial window width.
- `Window.height`: Initial window height.
- `Window.title`: Window title.
- `Window.fullscreen`: Whether to start in fullscreen mode.
- `Graphics.vsync`: Enable/disable vertical sync.

**Modular Functions:**
- `Window::initialize()` - Initializes GLFW, sets window hints.
- `Window::create(width, height, title, fullscreen)` - Creates the GLFW window and OpenGL context.
- `Window::shutdown()` - Destroys the window and terminates GLFW.
- `Window::shouldClose()` - Checks if the window should close.
- `Window::setShouldClose(bool)` - Sets the window close flag.
- `Window::pollEvents()` - Processes pending window events.
- `Window::swapBuffers()` - Swaps the front and back buffers and updates delta time.
- `Window::getGLFWwindow()` - Returns the raw `GLFWwindow*` handle.
- `Window::getWidth()` - Gets the window width.
- `Window::getHeight()` - Gets the window height.
- `Window::setTitle(title)` - Sets the window title.
- `Window::setVSync(enabled)` - Enables or disables vertical sync.
- `Window::getDeltaTime()` - Gets the time between frames for smooth animations.
- `Window::setResizeCallback(callback)` - Sets a callback function to handle window resizing.

**Example Usage (Integrated into `main.cpp`):**
```cpp
// Initialize GLFW
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

// Main loop
while (!window.shouldClose()) {
    // Get delta time for smooth animations
    float deltaTime = window.getDeltaTime();
    
    // Process events
    window.pollEvents();
    
    // ... update and render logic ...
    
    // Swap buffers
    window.swapBuffers();
}

// Cleanup
window.shutdown();
```

---

### 3.4. Camera System (`src/renderer/camera.h/.cpp`) ✅ COMPLETED

**Features:**
- Third-person orbit camera (fixed distance, orbits above/around player)
- **Implement mouse-driven camera movement around the player (Cube World style).** ✅

**Camera Parameters:**
```cpp
struct CameraParams {
    float orbitDistance = 5.0f;    // Distance from target
    float minDistance = 2.0f;      // Minimum zoom distance
    float maxDistance = 10.0f;     // Maximum zoom distance
    float targetHeight = 1.7f;     // Height of look-at point
    float rotationSpeed = 1.0f;    // Mouse rotation sensitivity
    float zoomSpeed = 1.0f;        // Zoom sensitivity
};
```

**Modular Functions:**
- `updateCameraOrbit(float deltaTime, const Player& player, const InputState& input)`
- `setCameraTargetHeight(float height)`
- `setCameraDistance(float distance)`
- `getCameraForward() const`

**Example Usage:**
```cpp
Camera camera;
camera.setTargetHeight(1.7f);  // Eye level
camera.setDistance(5.0f);      // Default distance
camera.updateOrbit(deltaTime, player, input);
```

---

### 3.5. Player System (`src/world/player.h/.cpp`) ✅ COMPLETED

**Features:**
- WASD world-space movement
- Rotates to face movement direction
- Basic collision with terrain
- 3D cuboid representation with distinct, identifiable faces (e.g., front, back, left, right, top, bottom).
- **Jump control (e.g., Spacebar).** ✅ COMPLETED

**🎮 Player movement goals (3rd person controller like Cube World):**
  - The player should move relative to the camera direction (not world direction).
  - Pressing W should move the player forward relative to the camera.
  - Pressing A should move left relative to the camera, and the player should rotate to face that new direction.
  - When the camera is rotated, the relative movement should adapt accordingly.
  - The camera orbits the player independently and always looks at the player (third-person view).

**👁️‍🗨️ Desired behavior summary:**
  - Movement is input-driven but transformed relative to the camera's forward/right vectors.
  - The player rotates to face the direction they're moving in.
  - Movement and orientation should feel intuitive and reactive, even when orbiting the camera.
  - (Note: Support for screen-centered crosshair aligned to the camera's forward direction is covered in UI Elements section 3.7)

**Player Parameters:**
```cpp
struct PlayerParams {
    float moveSpeed = 5.0f;        // Units per second
    float rotationSpeed = 2.0f;    // Radians per second
    float collisionRadius = 0.5f;  // Collision sphere radius
    float height = 1.8f;           // Player height (Y-axis)
    float jumpHeight = 3.0f;       // Maximum jump height 
    float jumpForce = 8.0f;        // Initial jump velocity
    float gravity = 15.0f;         // Gravity acceleration
    bool isGrounded = true;        // Whether the player is on the ground
    // 3D Rectangle (Cuboid) representation
    float width = 1.0f;            // Width of the cuboid (X-axis)
    float depth = 1.0f;            // Depth of the cuboid (Z-axis)
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};  // White color (RGBA)
};
```

**Modular Functions:**
- `updatePlayerMovement(float deltaTime, const InputState& input, const World& world)`
- `rotateToMovementDirection()`
- `checkCollision(const World& world)`
- `renderPlayer()`
- `setJumpHeight(float height)` 
- `setJumpForce(float force)`
- `setGravity(float gravity)`
- `isGrounded()`

**Example Usage:**
```cpp
Player player;
player.setMoveSpeed(5.0f);
player.setSize(1.0f, 1.0f);  // Set rectangle size
player.setColor({1.0f, 1.0f, 1.0f, 1.0f});  // Set to white
player.updateMovement(deltaTime, input, world);
player.renderPlayer();  // Render the rectangle
```

**Initial Implementation Notes:**
1. Implement the player as a 3D cuboid. The geometry should be defined in a way that allows for distinct faces (front, back, left, right, top, bottom) to be identified, potentially for future individual texturing or coloring.
2. Use basic OpenGL rendering (a single color for the initial cuboid, or distinct colors per face if straightforward).
3. The cuboid will represent the player's volume.
4. The vertex buffer for the cuboid should be organized to support face-specific attributes if needed.
5. Basic vertex and fragment shaders for solid color rendering (potentially supporting per-vertex or per-face colors if implemented).

**Basic Shader Example:**
```glsl
// basic.vert
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}

// basic.frag
#version 330 core
out vec4 FragColor;

uniform vec4 color;

void main() {
    FragColor = color;
}
```

---

### 3.6. World System (`src/world/world.h/.cpp`) ✅ COMPLETED

**Features:**
- Simple terrain (grid or voxel)
- Collision detection with player
- (Optional) Object placement/interactions

**World Parameters:**
```cpp
struct WorldParams {
    int terrainSize = 100;         // Size of terrain grid
    float cellSize = 1.0f;         // Size of each terrain cell
    float maxHeight = 10.0f;       // Maximum terrain height
    int seed = 0;                  // Terrain generation seed
};
```

**Modular Functions:**
- `generateTerrain()`
- `renderWorld(const Camera& camera)`
- `checkCollision(const Player& player)`
- `placeObject(const Object& obj, glm::vec3 position)`

**Example Usage:**
```cpp
World world;
world.setTerrainSize(100);
world.generateTerrain();
world.render(camera);
```

---

### 3.7. UI Elements (`src/renderer/ui.h/.cpp`) ✅ COMPLETED

**Features:**
- Screen-centered crosshair aligned with camera's forward vector
- (Extendable for other HUD elements)

**UI Parameters:**
```cpp
struct UIParams {
    float crosshairSize = 20.0f;   // Size in pixels
    glm::vec4 crosshairColor = {1.0f, 1.0f, 1.0f, 1.0f};  // RGBA
    float crosshairThickness = 2.0f;
};
```

**Modular Functions:**
- `renderCrosshair(const Camera& camera)`
- `renderHUD()`

**Example Usage:**
```cpp
UI::setCrosshairSize(20.0f);
UI::setCrosshairColor({1.0f, 1.0f, 1.0f, 1.0f});
UI::renderCrosshair(camera);
```

**Implementation Notes:**
- Created a crosshair using two quads forming a plus shape
- Used orthographic projection for 2D rendering in screen space
- Implemented proper OpenGL state management (depth test, blending)
- Added a static forward declaration for internal functions like `updateCrosshairGeometry()`
- Provided configuration options for crosshair size, thickness, and color

---

### 3.8. Audio System (`src/audio/audio_system.h/.cpp`) ✅ COMPLETED

**Features:**
- Sound playback and management system using OpenAL
- Support for different audio types (sound effects, music, ambient, voice)
- 3D positional audio linked to camera/player position
- Volume controls (master and per-type)
- Mute controls for specific audio types
- Resource management for audio files

**Audio Types:**
```cpp
enum class AudioType {
    SOUND_EFFECT,  // Short sounds played once or occasionally
    MUSIC,         // Background music tracks, typically looped
    AMBIENT,       // Environmental sounds, typically looped
    VOICE,         // Voice/dialogue audio
};
```

**Audio Parameters:**
```cpp
struct AudioParams {
    float masterVolume = 0.8f;      // Overall volume (0.0 to 1.0)
    float musicVolume = 0.7f;       // Music volume multiplier
    float sfxVolume = 1.0f;         // Sound effects volume multiplier
    float ambientVolume = 0.5f;     // Ambient sounds volume multiplier
    float voiceVolume = 1.0f;       // Voice/dialogue volume multiplier
    float distanceModel = 1.0f;     // Rolloff factor for distance attenuation
    int maxSources = 32;            // Maximum number of simultaneous sound sources
    bool muteMusic = false;         // Whether music is muted
    bool muteSfx = false;           // Whether sound effects are muted
    bool muteAmbient = false;       // Whether ambient sounds are muted
    bool muteVoice = false;         // Whether voice/dialogue is muted
};
```

**Modular Functions:**
- `AudioSystem::initialize()` - Initialize the audio system
- `AudioSystem::shutdown()` - Clean up and shut down audio
- `AudioSystem::loadSound(name, path, type)` - Load audio file
- `AudioSystem::playSound(name, loop, volume)` - Play sound (returns ID)
- `AudioSystem::stopSound(id)` - Stop a sound
- `AudioSystem::pauseSound(id)` - Pause playback
- `AudioSystem::resumeSound(id)` - Resume playback
- `AudioSystem::setMasterVolume(volume)` - Set overall volume
- `AudioSystem::setTypeVolume(type, volume)` - Set volume for a category
- `AudioSystem::setListenerPosition(position)` - Set position for 3D audio
- `AudioSystem::setTypeMuted(type, muted)` - Mute/unmute specific audio type
- `AudioSystem::isTypeMuted(type)` - Check if an audio type is muted
- `AudioSystem::muteAll()` - Mute all audio
- `AudioSystem::unmuteAll()` - Unmute all audio
- `AudioSystem::update()` - Update audio system each frame

**Example Usage:**
```cpp
// Initialize 
AudioSystem::initialize();

// Load sounds
AudioSystem::loadSound("footstep", "assets/audio/sfx/footstep.wav", AudioType::SOUND_EFFECT);
AudioSystem::loadSound("music", "assets/audio/music/background.ogg", AudioType::MUSIC);

// Play sounds
unsigned int musicId = AudioSystem::playSound("music", true, 0.7f); // Loop music at 70% volume
AudioSystem::playSound("footstep"); // Play once at default volume

// Update listener with camera position
float cameraPos[3] = { camera.getPosition().x, camera.getPosition().y, camera.getPosition().z };
AudioSystem::setListenerPosition(cameraPos);

// Volume controls
AudioSystem::setMasterVolume(0.8f);
AudioSystem::setTypeVolume(AudioType::MUSIC, 0.5f);

// Mute controls
AudioSystem::setTypeMuted(AudioType::MUSIC, true); // Mute just the music
AudioSystem::setTypeMuted(AudioType::MUSIC, false); // Unmute the music

// Cleanup
AudioSystem::stopSound(musicId);
AudioSystem::unloadAllSounds();
AudioSystem::shutdown();
```

---

### 3.9. Implementing Initial Rendering

This section focuses on implementing the rendering logic for the Player, World, and UI, and the foundational Shader system.

**3.9.1. Shader Management System (`src/renderer/shader.h/.cpp`)**

**Features:**
- Load, compile, and link vertex and fragment shaders from files.
- Activate/deactivate shader programs.
- Set uniform variables (e.g., model/view/projection matrices, colors).

**Modular Functions:**
- `Shader::Shader(const char* vertexPath, const char* fragmentPath)` - Constructor to load and compile.
- `Shader::use()` - Activate the shader program.
- `Shader::setBool(const std::string &name, bool value) const`
- `Shader::setInt(const std::string &name, int value) const`
- `Shader::setFloat(const std::string &name, float value) const`
- `Shader::setVec2(const std::string &name, const glm::vec2 &value) const`
- `Shader::setVec3(const std::string &name, const glm::vec3 &value) const`
- `Shader::setVec4(const std::string &name, const glm::vec4 &value) const`
- `Shader::setMat2(const std::string &name, const glm::mat2 &mat) const`
- `Shader::setMat3(const std::string &name, const glm::mat3 &mat) const`
- `Shader::setMat4(const std::string &name, const glm::mat4 &mat) const`
- `Shader::checkCompileErrors(GLuint shader, std::string type)` - Private helper.

**Example Usage (Conceptual):**
```cpp
// In rendering setup
Shader basicShader("assets/shaders/basic.vert", "assets/shaders/basic.frag");

// In render loop
basicShader.use();
basicShader.setMat4("projection", projectionMatrix);
basicShader.setMat4("view", viewMatrix);
basicShader.setMat4("model", modelMatrix);
basicShader.setVec4("color", objectColor);
// ... draw call ...
```

**3.9.2. Player Rendering (`src/world/player.h/.cpp`)**

**Task:** Implement the `renderPlayer()` method.
- Use the Shader system (e.g., `basic.vert`, `basic.frag` from `assets/shaders/`).
- Create a simple VAO/VBO for a rectangle or a basic 3D shape (e.g., a cube).
- Pass model, view, projection matrices and color to the shader.
- Initially, the player can be a simple colored shape.
- Consider billboard rendering if it's a 2D sprite in 3D space.

**Update Player Parameters (if needed for rendering):**
- Ensure `PlayerParams` or the `Player` class stores necessary data for its VBO/VAO.

**3.9.3. World Rendering (`src/world/world.h/.cpp`)**

**Task:** Implement the `renderWorld(const Camera& camera)` method.
- Use the Shader system.
- Render the terrain (grid or voxel representation).
- This will likely involve creating a VAO/VBO for the terrain mesh.
- Pass appropriate matrices and potentially material properties (like color) to shaders.

**3.9.4. UI Rendering (`src/renderer/ui.h/.cpp`) ✅ COMPLETED**

**Task:** Implement the `renderCrosshair(const Camera& camera)` method.
- Use the Shader system. This might require a separate shader for 2D orthographic rendering. ✅
- Create VAO/VBO for the crosshair geometry (e.g., two perpendicular lines). ✅
- Render in screen space. The `camera` argument might be used to get the forward vector for alignment if the crosshair is 3D-aware, or simply for consistency if the UI rendering is entirely separate. ✅
- Ensure projection is orthographic for UI elements. ✅
- Add proper forward declarations for internal functions to avoid compiler errors. ✅
- Use static helper functions for internal UI rendering operations like `updateCrosshairGeometry()`. ✅

**Implementation Notes:**
- Created a crosshair using two quads forming a plus shape
- Used orthographic projection for 2D rendering in screen space
- Implemented proper OpenGL state management (depth test, blending)
- Added a static forward declaration for internal functions like `updateCrosshairGeometry()`
- Provided configuration options for crosshair size, thickness, and color

**3.9.5. Update Main Application Flow (`src/main.cpp`) ✅ COMPLETED**

**Task:** Uncomment and integrate rendering calls.
- Initialize the Shader objects needed.
- In the main loop, after clearing the screen:
  1. Call `world.render(camera);`
  2. Call `player.render(camera);` (ensure player rendering takes into account its model matrix for position/rotation)
  3. Call `UI::renderCrosshair(camera);` (ensure UI is rendered last, typically with depth testing off or using an orthographic projection that places it on top).
- Ensure necessary graphics objects (VAOs, VBOs for player, world, UI) are initialized before the main loop and cleaned up afterwards.

---

### 3.10. Implement Player WASD Controls ✅ COMPLETED

This section details the implementation of WASD controls for player movement.

**3.10.1. Input System (`src/platform/input.h/.cpp`) ✅ COMPLETED**

- **Task:** Define and implement the `Input` system.
    - Create `input.h` and `input.cpp` in `src/platform/`. ✅
    - **Responsibilities:**
        - Capture raw keyboard input via GLFW callbacks (W, A, S, D keys). ✅
        - Maintain an `InputState` struct (already defined in `core/types.h`) reflecting current key presses for movement. ✅
        - Provide a way to access the current `InputState`. ✅
    - **Modular Functions (Examples):**
        - `namespace Input {` ✅
        - `  void initialize(GLFWwindow* window); // Sets up GLFW key callbacks` ✅
        - `  void update(); // Polls/processes key states (if not purely callback-driven)` ✅
        - `  const InputState& getState(); // Returns the current input state` ✅
        - `}` ✅

**3.10.2. Player Movement Logic (`src/world/player.h/.cpp`) ✅ COMPLETED**

- **Task:** Implement player movement in `Player::updatePlayerMovement(float deltaTime, const InputState& input, const World& world)`.
    - This function is already declared as a target in section 3.5 and has been implemented. ✅
    - **Implementation Details:**
        - Use `input.moveForward`, `input.moveBackward`, `input.moveLeft`, `input.moveRight` from the `InputState` provided by the `Input` system. ✅
        - Calculate a velocity vector based on these inputs and `playerParams.moveSpeed`. ✅
        - Update the player's position based on this velocity and `deltaTime`. ✅
        - Ensure movement is in "world-space" as per the Player System features. ✅
        - (Future task, can be separated: Implement "rotates to face movement direction".) ✅

**3.10.3. Integrate into Main Application Flow (`src/main.cpp`) ✅ COMPLETED**

- **Task:** Modify `main.cpp` to incorporate input processing and player movement updates.
    - After `Window::initialize()` and window creation, call `Input::initialize(window.getGLFWwindow())`. ✅
    - In the main loop, each frame:
        - Call `Input::update()` (or ensure GLFW callbacks are correctly updating the state accessible via `Input::getState()`). ✅
        - Retrieve the input state: `const InputState& currentInputState = Input::getState();` ✅
        - Call `player.updatePlayerMovement(deltaTime, currentInputState, world);` ✅
        - (Also ensure `camera.updateOrbit(deltaTime, player, currentInputState);` is called if camera movement depends on input). ✅

---

## 4. Main Application Flow (`src/main.cpp`)

**Responsibilities:**
- Initialize core systems (`Config`, `Logger`).
- Initialize GLFW and create a window.
- Initialize GLAD (OpenGL function loader).
- Set up OpenGL viewport and rendering state.
- Initialize other subsystems (`Camera`, `Player`, `World`, `UI`).
- Main loop:
  1. Process input (`glfwPollEvents()`).
  2. Update game logic (`Player`, `Camera`, `World`).
  3. Clear the screen (`glClear`).
  4. Render scene (`World`, `Player`).
  5. Render UI (`UI`).
  6. Swap buffers (`glfwSwapBuffers()`) and poll events.
- Clean up resources (`glfwTerminate()`).

**Example Skeleton with OpenGL Setup:**
```cpp
#include "core/config.h"
#include "core/logger.h"
// Include platform/renderer headers as needed
#include <glad/glad.h> // Must be included before GLFW
#include <GLFW/glfw3.h>
#include <iostream> // For error messages

// Placeholder includes - replace with actual paths
#include "platform/window.h" // Assuming this wraps GLFWwindow* or handles setup
#include "platform/input.h"
#include "renderer/camera.h"
#include "renderer/ui.h"
#include "world/player.h"
#include "world/world.h"

// Basic error callback for GLFW
void glfwErrorCallback(int error, const char* description) {
    Logger::logError("GLFW Error (" + std::to_string(error) + "): " + description);
}

int main() {
    // Initialize configuration
    Config::load("config/default_config.ini");
    Config::load("config/user_config.ini"); // override defaults

    // Initialize logger
    // Example: Logger::setLogLevel(Config::getString("Logging.level"));
    // Example: Logger::setLogFile(Config::getString("Logging.file"));
    Logger::logInfo("Engine starting...");

    // --- GLFW Initialization ---
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        Logger::logError("Failed to initialize GLFW");
        return -1;
    }
    Logger::logInfo("GLFW initialized.");

    // --- GLFW Window Hints (Example: OpenGL 3.3 Core Profile) ---
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on MacOS
#endif

    // --- Create GLFW Window ---
    int windowWidth = Config::getInt("Window.width");
    int windowHeight = Config::getInt("Window.height");
    const char* windowTitle = Config::getString("Window.title").c_str();

    GLFWwindow* glfwWindow = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
    if (!glfwWindow) {
        Logger::logError("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(glfwWindow);
    Logger::logInfo("GLFW window created.");

    // --- Initialize GLAD ---
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::logError("Failed to initialize GLAD");
        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
        return -1;
    }
    Logger::logInfo("GLAD initialized. OpenGL Version: " + std::string((const char*)glGetString(GL_VERSION)));

    // --- Set OpenGL Viewport ---
    glViewport(0, 0, windowWidth, windowHeight);
    // Optional: Set callback for window resize
    // glfwSetFramebufferSizeCallback(glfwWindow, framebuffer_size_callback);

    // Enable Depth Testing (Important for 3D)
    glEnable(GL_DEPTH_TEST);

    // --- Initialize Engine Subsystems ---
    // Note: Pass the GLFWwindow or necessary handles if required by subsystems
    // Example: Input::initialize(glfwWindow);
    Camera camera;
    Player player;
    World world;
    // Initialize player as white rectangle (or basic mesh)
    // player.initializeGraphics(); // Example: setup VAO/VBO
    // world.initializeGraphics(); // Example: setup terrain mesh

    Logger::logInfo("Subsystems initialized.");

    // --- Main Render Loop ---
    Logger::logInfo("Starting main loop...");
    while (!glfwWindowShouldClose(glfwWindow)) {
        // Calculate delta time (implement a proper timer)
        static float lastFrameTime = 0.0f;
        float currentFrameTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // --- Input ---
        glfwPollEvents();
        // Example: Input::processInput(deltaTime); // Update internal input state

        // --- Game Logic Update ---
        // player.updateMovement(deltaTime, Input::getState(), world); // Pass necessary data
        // camera.updateOrbit(deltaTime, player, Input::getState());   // Pass necessary data
        // world.update(deltaTime);

        // --- Rendering ---
        // 1. Clear Screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark grey background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. Set up shader programs and pass uniforms (MVP matrices, colors, etc.)
        // basicShader.use();
        // basicShader.setMat4("projection", projectionMatrix); // Calculate projection matrix
        // basicShader.setMat4("view", camera.getViewMatrix());   // Get view matrix from camera

        // 3. Render World (e.g., terrain)
        // world.render(camera); // Pass camera for view/projection matrices if needed by world's shader

        // 4. Render Player (e.g., the rectangle or a basic model)
        // player.render(camera); // Pass camera, set model matrix for player

        // 5. Render UI (e.g., crosshair) - Potentially with different shaders/projection
        // UI::renderCrosshair(camera); // Ensure UI renders in screen space

        // --- Swap Buffers and Poll Events ---
        glfwSwapBuffers(glfwWindow);
    }

    Logger::logInfo("Main loop finished.");

    // --- Cleanup ---
    // player.cleanupGraphics(); // Example
    // world.cleanupGraphics();  // Example
    // UI::cleanup();            // Example
    // Input::shutdown();        // Example

    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
    Logger::logInfo("GLFW terminated. Engine shutting down.");

    return 0;
}

---

## 5. Modularity & Clean Architecture ✅ COMPLETED

- **Encapsulate** logic in classes/functions per module.
- **Expose** only necessary interfaces in headers.
- **Minimize** inter-module dependencies (use forward declarations, interfaces).
- **Document** all public classes and methods (Doxygen style).

**Example of Clean Interface:**
```cpp
// Forward declarations
class Player;
class World;
struct InputState;

class Camera {
public:
    // Public interface
    void updateOrbit(float deltaTime, const Player& player, const InputState& input);
    glm::vec3 getPosition() const;
    glm::vec3 getForward() const;
    
private:
    // Implementation details
    CameraParams params;
    glm::vec3 position;
    glm::vec3 target;
    float yaw;
    float pitch;
};
```

---

## 6. Future Improvements

- **Character Animations:** Integrate skeletal animation for the player.
- **Advanced Terrain:** Implement procedural or LOD terrain, add biomes.
- **Physics:** Add a physics engine for realistic movement and collisions.
- **Interaction System:** Expand object placement, add item pickups, triggers.
- **Networking:** Add multiplayer support.
- **UI Expansion:** Health bars, minimap, inventory, etc.
- **Audio System:** Add sound effects and music.
- **Save/Load System:** Persist world and player state.

---

## 7. Voxel World Implementation: "Pelican Harbor" Stylized Aesthetic

This section outlines the steps to transform the world into a stylized micro-voxel environment, drawing inspiration from the "Pelican Harbor" aesthetic. The goal is a playful, handcrafted world with detailed terrain made of tiny, soft-edged cubes, contrasting with oversized, blocky characters.

### 7.1. Core Voxel Engine: Micro-Voxel Foundation

-   **Micro-Voxel Data Structure & Scale:**
    -   Implement a 3D array or an octree to store voxel data (e.g., `std::vector<std::vector<std::vector<BlockType>>>`).
    -   Define `BlockType` enum (e.g., `AIR`, `GRASS`, `DIRT`, `ROCK`, `WOOD`, `LEAVES`, `WATER`, `FLOWER_RED`, `FLOWER_YELLOW`).
    -   **Crucially, configure terrain voxels to be very small (micro-voxels) relative to the player character.** This increased block resolution is key to achieving detailed terrain.
    -   Consider options for "soft-edged" cubes, perhaps through shader techniques or subtle chamfering in the mesh generation if performance allows, to achieve a less harsh blocky look.
-   **Chunk System for Detailed Terrain:**
    -   Divide the world into chunks (e.g., 32x32x32 or potentially larger like 64x64x64, composed of micro-voxels) for efficient rendering and management.
    -   Implement dynamic loading/unloading of chunks as the player moves to support a potentially large and detailed world.
-   **Micro-Voxel Rendering & Optimization:**
    -   Create a mesh generator that converts chunk data with micro-voxels into a renderable mesh (VAO/VBO).
    -   Render small, densely packed cubes for terrain features, focusing on a high-resolution blocky look.
    -   **Aggressively implement greedy meshing** or similar culling techniques (e.g., only rendering visible faces) to manage the high number of micro-voxels and maintain performance.
    -   Ensure the rendering supports the "soft-edged" or slightly rounded look if pursued.

### 7.2. World Generation: Handcrafted & Playful Landscapes

-   **Detailed Procedural Generation for Organic Terrain:**
    -   Utilize multi-octave noise (e.g., Perlin, Simplex) for height maps, tuned for finer detail suitable for micro-voxels.
    -   Generate terrain that allows for natural-looking **slopes, layered ground, and varied elevations**, moving beyond simple chunky cubes.
    -   Convert height maps into a dense micro-voxel representation, ensuring terrain feels more sculpted and organic.
-   **Enhanced Terrain Features & "Alive" World:**
    -   Use the micro-voxel scale to create **lush forests** with more detailed trees, **colorful flower patches** (potentially new block types), and distinct **paths carved into the terrain**.
    -   Implement algorithms or placement strategies that make the world feel handcrafted and playful, rather than purely uniform.
    -   Vegetation should be blocky but benefit from the higher resolution of micro-voxels for more nuanced shapes.
-   **Structure & Biome Generation (Future Expansion):**
    -   Lay groundwork for generating more complex, handcrafted-feeling structures and diverse biomes, all utilizing the micro-voxel approach.

### 7.3. Voxel Aesthetics & Lighting: Warm, Cozy, and Playful

-   **Rich, Organic, Low-Poly Visual Style:**
    -   Aim for a rich, organic look where each block type (grass, dirt, rocks, etc.) contributes to the overall texture, while remaining **low-poly and readable**.
    -   The high density of micro-voxels should create a textured, almost "sculpted clay" feel, but individual blocks should still be discernible.
    -   Assign distinct, slightly desaturated or earthy base colors to `BlockType`s, fitting a natural, playful theme.
-   **Soft, Saturated Lighting for a Cozy Feel:**
    -   Implement a lighting model that produces **soft shadows** and a slightly **saturated color palette** to evoke a **warm, cozy, and inviting atmosphere**.
    -   Consider ambient occlusion (SSAO or voxel-based) to enhance the depth and handcrafted feel of the micro-voxel terrain.
    -   Directional light should be warm, and ambient light should contribute to the overall soft, playful mood.
-   **Shader Enhancements for Stylized Look:**
    -   Develop shaders that support the soft-edged look (if implemented), potentially with subtle gradients or color variations across block faces to reduce flatness.
    -   Shaders might also contribute to the "handcrafted" feel, perhaps with very subtle noise or imperfections in color.

### 7.4. Character Models: Oversized, Blocky, & Expressive

-   **Contrasting Character Voxel Scale:**
    -   Design player and NPC models using **significantly larger, more distinct voxel blocks** compared to the micro-voxels of the environment. This creates a strong visual contrast and emphasizes character presence.
-   **Expressive, Blocky Proportions & Vibrant Colors:**
    -   Characters should have **oversized, expressive proportions** (e.g., blocky birds, animals).
    -   Utilize **vibrant, appealing colors** for characters to make them stand out against the more natural tones of the environment.
    -   Models should be clearly block-based but allow for recognizable forms and expressions.
-   **Accessories & Personality:**
    -   Incorporate details like **tools, backpacks, or other accessories** made of similar large blocks to add personality to characters.
-   **NPC/Entity Models:**
    -   Extend this oversized, blocky, and expressive design philosophy to all other game entities for a consistent character art style.

### 7.5. World Interaction & Gameplay Feel

-   **Scale-Aware Interaction:**
    -   Design interaction mechanics (e.g., block breaking/placing, object interaction) that are intuitive given the significant size difference between micro-voxel terrain and larger character blocks.
    -   Player actions on the terrain might involve manipulating larger groups of micro-voxels or interacting with pre-defined "features" rather than individual tiny blocks, depending on gameplay goals.

### 7.6. Performance Optimization Strategies

-   **Advanced Meshing Algorithms:** Beyond greedy meshing, explore options like dual contouring or surface nets for smoother results from micro-voxels if the "soft-edged" look is a high priority and if performance allows. These can reduce vertex count for complex surfaces.
-   **Level of Detail (LOD) for Chunks:** Implement LOD for chunk meshes. Farther chunks can use simplified meshes or even impostors. This is crucial for managing the high detail of micro-voxel terrain over large distances.
-   **Voxel Culling:**
    -   **Frustum Culling:** Ensure chunks outside the camera's view frustum are not processed or rendered.
    -   **Occlusion Culling:** Implement techniques (e.g., software occlusion culling, hardware occlusion queries if supported) to avoid rendering chunks entirely hidden by other geometry (like large hills or structures).
-   **Optimized Voxel Data Access:**
    -   If using dense 3D arrays for chunk data, ensure memory access patterns are cache-friendly during mesh generation and voxel queries.
    -   If using octrees or other sparse structures, optimize node traversal and data retrieval.
-   **Multithreading/Asynchronous Operations:**
    -   Offload chunk generation (noise calculation, block placement) to worker threads.
    -   Perform mesh building for newly loaded or modified chunks on separate threads to prevent stuttering in the main render loop.
    -   Asynchronously load/save chunk data from/to disk.
-   **Shader Optimizations:**
    -   Keep voxel shaders (especially fragment shaders) as efficient as possible.
    -   Minimize texture lookups if using complex texturing per micro-voxel.
    -   Consider deferred shading if lighting becomes complex, though this adds its own overhead.
-   **Data Compression for Chunks:**
    -   For storing/transmitting chunk data (especially for large worlds or networking), consider compression techniques like Run-Length Encoding (RLE) for sequences of identical block types, or more advanced palette-based compression.
-   **Baked Lighting / Ambient Occlusion:**
    -   For static parts of the micro-voxel terrain, pre-calculate (bake) ambient occlusion or even full lighting into vertex colors or lightmap textures. This can significantly reduce real-time lighting calculations and enhance the handcrafted feel.
-   **Render Batching:** While greedy meshing inherently batches geometry within a chunk, ensure that rendering calls for multiple chunks are also efficient (e.g., minimizing state changes between draws).

---

**By adopting this "Pelican Harbor" inspired micro-voxel approach, the Sylva engine will aim for a unique and charming visual style, characterized by detailed, organic terrain and playful, oversized blocky characters, all contributing to a handcrafted and cozy game world.**

## 8. Micro-Voxel System Adaptation (cellSize = 0.1f)

With the world's `m_params.cellSize` now configured to `0.1f` (e.g., via `Config::getFloat("World.cell_size", 0.1f)`), a "micro-voxel" aesthetic is targeted. This change has significant implications across rendering, collision detection, physics, and debug visualizations. All systems interacting with voxel scale must be updated and verified.

### 8.1. Impact of `cellSize = 0.1f`

-   **Rendering:**
    -   **Visual Scale:** Voxel meshes must be generated or transformed to ensure individual voxels visually appear as `0.1 x 0.1 x 0.1` world units. This typically involves scaling vertex data in mesh generation or applying appropriate scaling in the model matrix during rendering.
    -   **Performance:** Rendering a significantly larger number of smaller voxels can heavily impact performance. Greedy meshing, chunk LODs, and culling techniques become even more critical.
    -   **Detail:** Allows for much finer-grained detail in terrain and structures.

-   **Collision Detection:**
    -   **Precision:** Collision checks become more precise, capable of detecting interactions with smaller features.
    -   **Complexity & Performance:** Player bounding box sampling (`VoxelWorld::checkCollision`) needs denser sampling (smaller step sizes relative to `cellSize`) which can increase computational cost.
    -   **Snagging:** Increased risk of player snagging on minute voxel edges if collision response is not smooth.

-   **Physics (General):**
    -   **Stability:** Raycasting, ground detection (`VoxelWorld::getHeightAt`), and any other physics-based calculations (e.g., projectile trajectories, voxel-based interactions) must use the correct `cellSize` for accuracy and stability.
    -   **Interactions:** Mechanics like block breaking/placing now operate on much smaller units, which might require adjustments to feel and game balance.

-   **Debug Visualizations:**
    -   **Scale:** All debug rendering (AABBs for voxels, collision points, raycasts) must be scaled according to the new `cellSize` to be meaningful. For example, an AABB for a voxel should be `0.1 x 0.1 x 0.1`.

### 8.2. Checklist for System Adaptation and Verification

**Rendering & World Generation:**
-   `[X]` Confirm `m_params.cellSize` is consistently loaded/used in `VoxelWorld` and `Chunk` (e.g., from config, defaulting to `0.1f`).
-   `[X]` **Verify `Chunk::render` applies `cellSize` scaling to its model matrix to correctly size voxels visually.** (This was the primary fix for visual scale).
-   `[ ]` Ensure `Chunk::addFaceToMesh` (or equivalent mesh generation logic) produces vertices that, when combined with the chunk's model matrix, result in `0.1`-sized voxels.
-   `[ ]` Review and optimize greedy meshing algorithms for micro-voxels.
-   `[ ]` Plan/Implement Level of Detail (LOD) for chunks to manage performance with high voxel density.
-   `[ ]` Stress-test rendering performance with large areas of micro-voxels.

**Coordinate Systems & Conversions:**
-   `[X]` Verify `Chunk::worldToChunkPos()`, `Chunk::chunkToWorldPos()`, `Chunk::worldToLocalPos()` correctly use `cellSize`.
-   `[ ]` Audit any other voxel-to-world or world-to-voxel coordinate conversions throughout the codebase.

**Collision Detection:**
-   `[X]` Update iteration step sizes in `VoxelWorld::checkCollision` to be relative to `m_params.cellSize` (e.g., `m_params.cellSize * 0.5f`).
-   `[ ]` Test player collision thoroughly on flat surfaces, slopes, edges, and thin (single micro-voxel) structures.
-   `[ ]` Evaluate and refine player collision response to prevent snagging and jittering.
-   `[ ]` If explicit AABBs are generated for voxels (e.g., for broadphase), ensure they are dimensioned as `cellSize x cellSize x cellSize`.

**Physics & Interactions:**
-   `[X]` Review and update `VoxelWorld::getHeightAt()` to use appropriate step sizes (e.g., `m_params.cellSize`) for accurate ground detection with micro-voxels.
-   `[ ]` Update any raycasting logic (e.g., mouse picking, targeting) to correctly traverse the denser micro-voxel grid.
-   `[ ]` Consider if Continuous Collision Detection (CCD) principles are needed if player/object speeds are high relative to `cellSize`.
-   `[ ]` Test game mechanics like block breaking/placing to ensure they function correctly and feel appropriate at the new scale.

**Collision Debug System (`VoxelWorld::renderCollisionDebug`):**
-   `[ ]` If rendering AABBs for colliding voxels:
    -   `[ ]` Ensure the debug AABBs are drawn with dimensions `cellSize x cellSize x cellSize`.
    -   `[ ]` Ensure the AABBs are positioned correctly at the world coordinates of the colliding micro-voxel.
-   `[ ]` If rendering collision points:
    -   `[X]` Confirm points are generated based on `cellSize`-aware collision checks (already done in `checkCollision`).
    -   `[ ]` Ensure point size (`glPointSize`) is visually appropriate for the new scale.
-   `[ ]` If rendering raycasts, ensure visualization correctly reflects ray traversal through micro-voxels.

**General:**
-   `[ ]` Audit any system component that queries voxel data (`getBlockAt`, etc.) or assumes a grid structure, ensuring no hardcoded assumptions about block sizes.
-   `[ ]` Profile performance impacts across all affected systems (rendering, collision, physics) and identify areas for optimization.

This systematic update and verification process is crucial for ensuring the stability, correctness, and performance of the engine with the new micro-voxel scale.

## 9. Stylized Visual Enhancements (Pelican Harbor Aesthetic)

This section details the implementation of visual features to achieve a soft, vibrant, and handcrafted look reminiscent of "Pelican Harbor," building upon the micro-voxel foundation.

### 9.1. Core Principles
- **Efficiency:** Prioritize performance. Techniques should be suitable for real-time rendering on typical hardware, leveraging greedy meshing and avoiding expensive post-processing where possible.
- **Stylized Realism:** Aim for a look that is visually appealing and conveys detail without being photorealistic.
- **Cohesive Aesthetic:** Ensure all visual elements (lighting, color, AO) work together to create the desired mood.

### 9.2. Soft Voxel Lighting
-   **Goal:** Implement a per-fragment Lambertian diffuse model combined with ambient light to give voxels a soft, illuminated appearance.
-   **Implementation:**
    -   `[X]` **Vertex Shader (`voxel.vert`):**
        -   `[X]` Pass world-space vertex position to the fragment shader.
        -   `[X]` Transform and pass world-space normal to the fragment shader.
    -   `[X]` **Fragment Shader (`voxel.frag`):**
        -   `[X]` Add uniforms: `vec3 lightDir`, `vec3 lightColor`, `vec3 ambientColor`.
        -   `[X]` Calculate Lambertian diffuse term: `max(dot(normalize(NormalWorld), normalize(lightDir)), 0.0) * lightColor`.
        -   `[X]` Final color = `(ambientColor + diffuseTerm) * baseVoxelColor`.
    -   `[X]` **C++ (`VoxelWorld::render` or similar):**
        -   `[X]` Set the new light uniforms (e.g., a fixed directional light, configurable ambient intensity).

### 9.3. Fake Ambient Occlusion (Vertex-Based)
-   **Goal:** Darken voxel vertices that are surrounded by neighboring voxels to simulate self-shadowing and improve depth perception.
-   **Implementation:**
    -   `[X]` **Mesh Generation (`Chunk::addFaceToMesh`):**
        -   `[X]` For each vertex of a face being added:
            -   `[X]` Identify its 3-7 potential occluding neighbor voxels (depending on whether it's a corner, edge, or face vertex relative to the block being meshed, and considering the 8 voxels that meet at a vertex point).
            -   `[X]` Count how many of these potential occluders are solid blocks.
            -   `[X]` Calculate an occlusion factor (e.g., `1.0 - (num_solid_neighbors / max_possible_occluding_neighbors)` or a simpler 0-1 per occluding neighbor sum).
        -   `[X]` Add this occlusion factor (float) as a new vertex attribute.
        -   `[X]` Update vertex stride and attribute pointers in `Chunk::generateMesh`.
    -   `[X]` **Vertex Shader (`voxel.vert`):**
        -   `[X]` Pass the occlusion factor to the fragment shader.
    -   `[X]` **Fragment Shader (`voxel.frag`):**
        -   `[X]` Modulate the ambient light component (and optionally diffuse) by the occlusion factor: `finalAmbient = ambientColor * occlusionFactor`.

### 9.4. Slight Hue/Color Variation Per Voxel
-   **Goal:** Introduce subtle, position-based color variations to voxels to prevent a flat, uniform appearance and enhance the handcrafted feel.
-   **Implementation:**
    -   `[X]` **Mesh Generation (`Chunk::addFaceToMesh`):**
        -   `[X]` The existing color calculation already includes a `colorVariation` based on `vx, vy, vz`. Review and refine this.
        -   `[X]` To achieve more distinct "hue" variation, consider converting the base block color to HSL/HSV, slightly shifting the Hue based on a hash of the voxel's world/chunk position, and then converting back to RGB.
        -   `[X]` Alternatively, apply small, bounded random offsets to R, G, B channels based on a deterministic hash of the voxel's absolute position (world_x, world_y, world_z passed to `addFaceToMesh` or calculated from local x,y,z and chunk position).
        -   `[X]` Ensure the variation is subtle and complements the base color rather than drastically changing it.
        -   `[X]` The varied color should be part of the vertex color attribute.
    -   `[X]` **Fragment Shader (`voxel.frag`):**
        -   `[X]` No direct change needed if the variation is baked into vertex colors, unless specific shader logic is preferred to apply the variation.

### 9.5. Shader and Vertex Attribute Summary
- **New Vertex Attributes (example order):**
    1. Position (vec3)
    2. Color (vec3) - Potentially with baked hue variation
    3. UV Coordinates (vec2)
    4. Normal (vec3)
    5. Ambient Occlusion Factor (float)
- Update total vertex stride and `glVertexAttribPointer` calls in `Chunk::generateMesh` accordingly.

### 9.6. Testing and Iteration
-   `[ ]` Test each feature incrementally.
-   `[ ]` Adjust lighting parameters, AO strength, and hue variation intensity to achieve the desired aesthetic.
-   `[ ]` Profile performance, especially after adding AO calculations to mesh generation.

### 9.7. Fix Visual Artifacts on Block Edges
-   **Goal:** Eliminate incorrect coloring or lighting artifacts appearing at the edges of blocks/quads.
-   **Investigation:**
    -   `[ ]` Review how vertex attributes (color with hue variation, ambient occlusion factor) are calculated and applied in `Chunk::addFaceToMesh`, especially for vertices of greedy-meshed quads.
    -   `[ ]` Ensure that hue variation and AO are determined based on the unique world position of each *vertex*, not just the base voxel of a greedy quad.
-   **Implementation Strategy:**
    -   `[ ]` Modify `Chunk::addFaceToMesh` (and potentially `BlockData` or helper functions) to calculate color and AO per-vertex using the vertex's world grid coordinates.
    -   `[ ]` Ensure vertex normals are consistent and correctly calculated/retrieved for lighting.
    -   `[ ]` Test thoroughly after changes.
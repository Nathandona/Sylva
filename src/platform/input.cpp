#include "input.h"
#include "core/logger.h"
#include <glad/glad.h> // Include GLAD before GLFW
#include <GLFW/glfw3.h>

namespace Sylva::Input {

namespace {

// Internal module state. Anonymous namespace gives internal linkage
// without the C-style 'static' keyword.
InputState s_inputState;
GLFWwindow* s_window = nullptr;
bool s_cursorDisabled = true; // Track cursor state

// Key callback function for GLFW.
void keyCallback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/) {
    const bool down = (action != GLFW_RELEASE); // true on press OR repeat, false on release
    switch (key) {
    // Movement controls
    case GLFW_KEY_W:
        s_inputState.moveForward = down;
        break;
    case GLFW_KEY_S:
        s_inputState.moveBackward = down;
        break;
    case GLFW_KEY_A:
        s_inputState.moveLeft = down;
        break;
    case GLFW_KEY_D:
        s_inputState.moveRight = down;
        break;

    // Additional controls
    case GLFW_KEY_SPACE:
        s_inputState.jump = down;
        break;
    case GLFW_KEY_E:
        s_inputState.interact = down;
        break;
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:
        s_inputState.sprint = down;
        break;

    // Audio controls
    case GLFW_KEY_UP:
        s_inputState.volumeUp = down;
        break;
    case GLFW_KEY_DOWN:
        s_inputState.volumeDown = down;
        break;
    case GLFW_KEY_M:
        s_inputState.mute = down;
        break;

    // Cursor toggle (only on press, not hold)
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS) {
            s_inputState.toggleCursor = true;
        }
        break;
    default:
        break;
    }
}

// Mouse button callback.
void mouseButtonCallback(GLFWwindow* /*window*/, int button, int action, int /*mods*/) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        s_inputState.mouseLeftButton = (action == GLFW_PRESS);
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        s_inputState.mouseRightButton = (action == GLFW_PRESS);
    }
}

// Mouse movement callback.
void cursorPosCallback(GLFWwindow* /*window*/, double xpos, double ypos) {
    static double lastX = xpos;
    static double lastY = ypos;
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // Calculate delta
    s_inputState.mouseDeltaX = static_cast<float>(xpos - lastX);
    s_inputState.mouseDeltaY = static_cast<float>(lastY - ypos); // Reversed since y-coordinates go from bottom to top

    // Update last position
    lastX = xpos;
    lastY = ypos;
}

// Scroll callback for mouse wheel input.
void scrollCallback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    s_inputState.mouseWheelDelta = static_cast<float>(yoffset);
}

} // namespace

void initialize(GLFWwindow* window) {
    Logger::logInfo("Initializing Input system");

    if (window == nullptr) {
        Logger::logError("Failed to initialize Input system: window is null");
        return;
    }

    s_window = window;

    // Reset input state
    s_inputState = InputState();

    // Set callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback); // Add scroll callback

    // Set cursor mode for camera control (Cube World style)
    toggleCursorMode(true); // Start with cursor disabled

    Logger::logInfo("Input system initialized");
}

void update() {
    if (s_window == nullptr) {
        return;
    }

    // Check for cursor lock/unlock toggle (ESC key)
    if (s_inputState.toggleCursor) {
        toggleCursorMode(!s_cursorDisabled);
        s_inputState.toggleCursor = false; // Reset the flag
    }

    // Reset mouse delta values after they've been used for this frame
    s_inputState.mouseDeltaX = 0.0f;
    s_inputState.mouseDeltaY = 0.0f;
    s_inputState.mouseWheelDelta = 0.0f;
}

void toggleCursorMode(bool disabled) {
    if (s_window == nullptr)
        return;

    if (disabled) {
        // Disable cursor for camera control
        glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        Logger::logDebug("Cursor disabled for camera control");
    } else {
        // Enable cursor for UI interaction
        glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        Logger::logDebug("Cursor enabled for UI interaction");
    }

    s_cursorDisabled = disabled;
}

const InputState& getState() {
    return s_inputState;
}

bool isKeyPressed(int key) {
    if (s_window == nullptr)
        return false;
    return glfwGetKey(s_window, key) == GLFW_PRESS;
}

void shutdown() {
    Logger::logInfo("Shutting down Input system");
    s_window = nullptr;
}

void reset() {
    s_inputState = InputState{};
    s_window = nullptr;
    s_cursorDisabled = true;
}

} // namespace Sylva::Input

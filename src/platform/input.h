#pragma once

#include "core/types.h"

// Forward declaration instead of including the full header
struct GLFWwindow;

namespace Sylva {

/**
 * @brief Input system
 *
 * Handles user input through GLFW and maintains the current input state.
 */
namespace Input {

/**
 * @brief Initialize the input system
 * @param window GLFW window to capture input from
 */
void initialize(GLFWwindow* window);

/**
 * @brief Update the input state (called each frame)
 */
void update();

/**
 * @brief Get the current input state
 * @return Reference to the current input state
 */
const InputState& getState();

/**
 * @brief Check if a specific key is currently pressed
 * @param key GLFW key code to check
 * @return True if the key is pressed, false otherwise
 */
bool isKeyPressed(int key);

/**
 * @brief Toggle cursor mode between disabled (for camera control) and normal
 * @param disabled Whether to disable the cursor
 */
void toggleCursorMode(bool disabled);

/**
 * @brief Shutdown the input system
 */
void shutdown();

/**
 * @brief Reset module state (input state struct, window pointer, cursor flag).
 * Provided so tests can isolate themselves from a previous run.
 */
void reset();

} // namespace Input

} // namespace Sylva
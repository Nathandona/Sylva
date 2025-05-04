#pragma once

#include <string>
#include <unordered_map>
#include "../platform/Platform.h"

namespace Sylva {

// Forward declarations
class Platform;

// Defines an action that can be triggered by input
struct InputAction {
    std::string name;        // Name of the action
    int keyCode;             // Primary key code (GLFW_KEY_*)
    int alternateKeyCode;    // Alternate key code (or -1 if none)
    int mouseButton;         // Mouse button (or -1 if not used)
};

// Manages input mapping and provides an abstraction layer over raw key checks
class InputManager {
public:
    InputManager(Platform* platform);
    ~InputManager() = default;

    // Initialize with default input mappings
    void Initialize();

    // Check if an action is currently being performed
    bool IsActionPressed(const std::string& actionName) const;

    // Get axis value (-1.0 to 1.0) for a pair of actions (e.g., "MoveForward"/"MoveBackward")
    float GetAxisValue(const std::string& positiveAction, const std::string& negativeAction) const;

    // Get the mouse scroll delta
    float GetScrollDelta() const;

    // Register a custom action mapping
    void RegisterAction(const std::string& actionName, int keyCode, int alternateKeyCode = -1, int mouseButton = -1);

    // Get/Set the mouse sensitivity
    float GetMouseSensitivity() const { return m_MouseSensitivity; }
    void SetMouseSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; }

    // Get mouse movement delta (call once per frame)
    void GetMouseDelta(float& deltaX, float& deltaY);

    // Check if right mouse button is pressed (for camera control)
    bool IsRightMouseButtonPressed() const;

private:
    // Reference to the platform system
    Platform* m_Platform;

    // Map of action names to their input mappings
    std::unordered_map<std::string, InputAction> m_Actions;

    // Mouse state tracking
    double m_LastMouseX;
    double m_LastMouseY;
    bool m_FirstMouse;
    float m_MouseSensitivity;
};

} // namespace Sylva 
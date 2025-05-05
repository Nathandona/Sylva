#include "InputManager.h"
#include <GLFW/glfw3.h>
#include "../core/Logger.h"

namespace Sylva {

InputManager::InputManager(Platform* platform)
    : m_Platform(platform)
    , m_LastMouseX(0.0)
    , m_LastMouseY(0.0)
    , m_FirstMouse(true)
    , m_MouseSensitivity(0.2f)
{
}

void InputManager::Initialize() {
    // Register default movement actions
    RegisterAction("MoveForward", GLFW_KEY_W, -1, -1);
    RegisterAction("MoveBackward", GLFW_KEY_S, -1, -1);
    RegisterAction("MoveLeft", GLFW_KEY_A, -1, -1);
    RegisterAction("MoveRight", GLFW_KEY_D, -1, -1);
    RegisterAction("Jump", GLFW_KEY_SPACE, -1, -1);
    RegisterAction("Run", GLFW_KEY_LEFT_SHIFT, -1, -1);
    
    // Register camera control actions
    RegisterAction("TurnLeft", GLFW_KEY_Q, GLFW_KEY_LEFT, -1);
    RegisterAction("TurnRight", GLFW_KEY_E, GLFW_KEY_RIGHT, -1);
    RegisterAction("LookUp", GLFW_KEY_UP, -1, -1);
    RegisterAction("LookDown", GLFW_KEY_DOWN, -1, -1);
    RegisterAction("ZoomIn", GLFW_KEY_PAGE_UP, -1, -1);
    RegisterAction("ZoomOut", GLFW_KEY_PAGE_DOWN, -1, -1);
    
    // New actions for camera features from plan
    RegisterAction("SwitchCameraMode", GLFW_KEY_C, -1, -1);
    RegisterAction("ToggleShoulder", GLFW_KEY_TAB, -1, -1);
    
    LOG_INFO("InputManager initialized with default mappings");
}

bool InputManager::IsActionPressed(const std::string& actionName) const {
    auto it = m_Actions.find(actionName);
    if (it == m_Actions.end()) {
        return false;
    }
    
    const InputAction& action = it->second;
    
    // Check primary key
    if (action.keyCode != -1 && m_Platform->IsKeyPressed(action.keyCode)) {
        return true;
    }
    
    // Check alternate key
    if (action.alternateKeyCode != -1 && m_Platform->IsKeyPressed(action.alternateKeyCode)) {
        return true;
    }
    
    // Check mouse button
    if (action.mouseButton != -1 && m_Platform->IsMouseButtonPressed(action.mouseButton)) {
        return true;
    }
    
    return false;
}

float InputManager::GetAxisValue(const std::string& positiveAction, const std::string& negativeAction) const {
    float value = 0.0f;
    
    if (IsActionPressed(positiveAction)) {
        value += 1.0f;
    }
    
    if (IsActionPressed(negativeAction)) {
        value -= 1.0f;
    }
    
    return value;
}

float InputManager::GetScrollDelta() const {
    return m_Platform->GetMouseScrollOffset();
}

void InputManager::RegisterAction(const std::string& actionName, int keyCode, int alternateKeyCode, int mouseButton) {
    InputAction action;
    action.name = actionName;
    action.keyCode = keyCode;
    action.alternateKeyCode = alternateKeyCode;
    action.mouseButton = mouseButton;
    
    m_Actions[actionName] = action;
}

void InputManager::GetMouseDelta(float& deltaX, float& deltaY) {
    double mouseX, mouseY;
    m_Platform->GetMousePosition(mouseX, mouseY);
    
    if (m_FirstMouse) {
        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;
        m_FirstMouse = false;
        deltaX = 0.0f;
        deltaY = 0.0f;
        return;
    }
    
    // Calculate movement delta
    deltaX = static_cast<float>(mouseX - m_LastMouseX) * m_MouseSensitivity;
    deltaY = static_cast<float>(m_LastMouseY - mouseY) * m_MouseSensitivity; // Reversed since y-coordinates range from bottom to top
    
    // Update last position
    m_LastMouseX = mouseX;
    m_LastMouseY = mouseY;
}

bool InputManager::IsRightMouseButtonPressed() const {
    return m_Platform->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
}

} // namespace Sylva 
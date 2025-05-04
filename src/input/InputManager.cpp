#include "InputManager.h"
#include <GLFW/glfw3.h>

namespace Sylva {

InputManager::InputManager(Platform* platform)
    : m_Platform(platform)
    , m_FirstMouse(true)
    , m_LastMouseX(0.0)
    , m_LastMouseY(0.0)
    , m_MouseSensitivity(0.1f)
{
}

void InputManager::Initialize() {
    // Player movement
    RegisterAction("MoveForward", GLFW_KEY_W);
    RegisterAction("MoveBackward", GLFW_KEY_S);
    RegisterAction("MoveLeft", GLFW_KEY_A);
    RegisterAction("MoveRight", GLFW_KEY_D);
    RegisterAction("Jump", GLFW_KEY_SPACE);
    RegisterAction("Run", GLFW_KEY_LEFT_SHIFT);
    
    // Player rotation
    RegisterAction("TurnLeft", GLFW_KEY_Q);
    RegisterAction("TurnRight", GLFW_KEY_E);
    
    // Camera controls
    RegisterAction("CameraPitchUp", GLFW_KEY_UP);
    RegisterAction("CameraPitchDown", GLFW_KEY_DOWN);
    RegisterAction("ZoomIn", GLFW_KEY_PAGE_UP);
    RegisterAction("ZoomOut", GLFW_KEY_PAGE_DOWN);
    RegisterAction("ToggleShoulder", GLFW_KEY_TAB);
    RegisterAction("CameraOrbit", -1, -1, GLFW_MOUSE_BUTTON_RIGHT);
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
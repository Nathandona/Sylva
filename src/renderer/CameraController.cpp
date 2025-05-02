#include "CameraController.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

namespace Sylva {

CameraController::CameraController(Camera* camera, Platform* platform)
    : m_Camera(camera)
    , m_Platform(platform)
    , m_ControlMode(ControlMode::FirstPerson)
    , m_TargetPosition(glm::vec3(0.0f))
    , m_OrbitDistance(5.0f)
    , m_FirstMouse(true)
    , m_LastMouseX(0.0)
    , m_LastMouseY(0.0)
    , m_MovementSpeed(5.0f)
    , m_MouseSensitivity(0.1f)
{
    // Initial camera setup
    double mouseX, mouseY;
    m_Platform->GetMousePosition(mouseX, mouseY);
    m_LastMouseX = mouseX;
    m_LastMouseY = mouseY;
}

void CameraController::SetControlMode(ControlMode mode) {
    m_ControlMode = mode;
    
    // Reset mouse state when switching modes
    m_FirstMouse = true;
}

void CameraController::SetTargetPosition(const glm::vec3& targetPosition) {
    m_TargetPosition = targetPosition;
    
    // If in third-person/orbit mode, update camera position
    if (m_ControlMode == ControlMode::ThirdPerson || m_ControlMode == ControlMode::Orbit) {
        // Position the camera at a distance behind and slightly above the target
        glm::vec3 offset;
        if (m_ControlMode == ControlMode::ThirdPerson) {
            float cameraYaw = m_Camera->GetYaw();
            float cameraPitch = m_Camera->GetPitch();
            
            // Calculate offset based on current camera rotation
            offset.x = -cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
            offset.y = sin(glm::radians(cameraPitch));
            offset.z = -sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
            
            offset = glm::normalize(offset) * m_OrbitDistance;
        } else {
            // In orbit mode, position is based on angles
            float cameraYaw = m_Camera->GetYaw();
            float cameraPitch = m_Camera->GetPitch();
            
            offset.x = m_OrbitDistance * cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
            offset.y = m_OrbitDistance * sin(glm::radians(cameraPitch));
            offset.z = m_OrbitDistance * sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        }
        
        m_Camera->SetPosition(m_TargetPosition + offset);
        m_Camera->SetTarget(m_TargetPosition);
    }
}

void CameraController::SetOrbitDistance(float distance) {
    m_OrbitDistance = std::max(0.1f, distance);
    
    // Update camera position if in third-person/orbit mode
    if (m_ControlMode == ControlMode::ThirdPerson || m_ControlMode == ControlMode::Orbit) {
        SetTargetPosition(m_TargetPosition);
    }
}

void CameraController::Update(float deltaTime) {
    // Process input based on control mode
    switch (m_ControlMode) {
        case ControlMode::FirstPerson:
            HandleFirstPersonInput(deltaTime);
            break;
        case ControlMode::ThirdPerson:
            HandleThirdPersonInput(deltaTime);
            break;
        case ControlMode::Orbit:
            HandleOrbitInput(deltaTime);
            break;
    }
    
    // Handle mouse movement for camera rotation
    HandleMouseMovement();
}

void CameraController::OnWindowResize(int width, int height) {
    // Update camera's aspect ratio
    if (width > 0 && height > 0) {
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        m_Camera->SetAspectRatio(aspectRatio);
    }
}

void CameraController::HandleFirstPersonInput(float deltaTime) {
    // Keyboard input for movement
    float velocity = m_MovementSpeed * deltaTime;
    
    // Check W, A, S, D keys for movement
    if (m_Platform->IsKeyPressed(GLFW_KEY_W)) {
        m_Camera->MoveForward(velocity);
    }
    if (m_Platform->IsKeyPressed(GLFW_KEY_S)) {
        m_Camera->MoveForward(-velocity);
    }
    if (m_Platform->IsKeyPressed(GLFW_KEY_A)) {
        m_Camera->MoveRight(-velocity);
    }
    if (m_Platform->IsKeyPressed(GLFW_KEY_D)) {
        m_Camera->MoveRight(velocity);
    }
    
    // Additional movement keys (optional)
    if (m_Platform->IsKeyPressed(GLFW_KEY_SPACE)) {
        m_Camera->MoveUp(velocity);
    }
    if (m_Platform->IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
        m_Camera->MoveUp(-velocity);
    }
}

void CameraController::HandleThirdPersonInput(float deltaTime) {
    // In third-person mode, we typically move the target (player character)
    // For now, let's just orbit around a fixed target
    
    // Allow zooming in/out
    float velocity = m_MovementSpeed * deltaTime;
    
    if (m_Platform->IsKeyPressed(GLFW_KEY_Q)) {
        m_OrbitDistance += velocity;
    }
    if (m_Platform->IsKeyPressed(GLFW_KEY_E)) {
        m_OrbitDistance = std::max(0.1f, m_OrbitDistance - velocity);
    }
    
    // Update camera position
    SetTargetPosition(m_TargetPosition);
}

void CameraController::HandleOrbitInput(float deltaTime) {
    // In orbit mode, the camera rotates around a fixed point
    float velocity = m_MovementSpeed * deltaTime;
    
    // Allow zooming in/out
    if (m_Platform->IsKeyPressed(GLFW_KEY_Q)) {
        m_OrbitDistance += velocity;
    }
    if (m_Platform->IsKeyPressed(GLFW_KEY_E)) {
        m_OrbitDistance = std::max(0.1f, m_OrbitDistance - velocity);
    }
    
    // Update camera position
    SetTargetPosition(m_TargetPosition);
}

void CameraController::HandleMouseMovement() {
    // Only process mouse movement if right mouse button is pressed (enables mouselook)
    if (!m_Platform->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        m_FirstMouse = true; // Reset first mouse flag when releasing the button
        return;
    }
    
    // Get current mouse position
    double mouseX, mouseY;
    m_Platform->GetMousePosition(mouseX, mouseY);
    
    // First mouse movement
    if (m_FirstMouse) {
        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;
        m_FirstMouse = false;
        return;
    }
    
    // Calculate offset
    float xOffset = static_cast<float>(mouseX - m_LastMouseX);
    float yOffset = static_cast<float>(m_LastMouseY - mouseY); // Reversed: y-coordinates go from bottom to top
    
    // Update last mouse position
    m_LastMouseX = mouseX;
    m_LastMouseY = mouseY;
    
    // Process mouse movement
    m_Camera->ProcessMouseMovement(xOffset, yOffset);
    
    // For third-person/orbit mode, update the camera position
    if (m_ControlMode == ControlMode::ThirdPerson || m_ControlMode == ControlMode::Orbit) {
        SetTargetPosition(m_TargetPosition);
    }
}

} // namespace Sylva 
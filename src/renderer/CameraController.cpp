#include "CameraController.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

namespace Sylva {

CameraController::CameraController(Camera* camera, Platform* platform)
    : m_Camera(camera)
    , m_Platform(platform)
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
    
    // Set initial rotation to look slightly downward
    m_Camera->SetRotation(-30.0f, -180.0f);
}

void CameraController::SetTargetPosition(const glm::vec3& targetPosition) {
    m_TargetPosition = targetPosition;
    
    // Position the camera at a distance behind and slightly above the target
    glm::vec3 offset;
    
    // Calculate camera position based on yaw (horizontal rotation around player)
    float cameraYaw = m_Camera->GetYaw();
    float cameraPitch = m_Camera->GetPitch();
    
    // Convert yaw to radians and calculate offset
    float yawRadians = glm::radians(cameraYaw);
    float pitchRadians = glm::radians(cameraPitch);
    
    // Position camera based on yaw and pitch angles for better 3D control
    offset.x = -m_OrbitDistance * sin(yawRadians) * cos(pitchRadians);
    offset.y = m_OrbitDistance * sin(pitchRadians);
    offset.z = -m_OrbitDistance * cos(yawRadians) * cos(pitchRadians);
    
    // Set camera position and make it look at the target
    m_Camera->SetPosition(m_TargetPosition + offset);
    m_Camera->SetTarget(m_TargetPosition);
}

void CameraController::SetOrbitDistance(float distance) {
    m_OrbitDistance = std::max(0.1f, distance);
    
    // Update camera position
    SetTargetPosition(m_TargetPosition);
}

void CameraController::Update(float deltaTime) {
    // Process input for third-person mode
    HandleThirdPersonInput(deltaTime);
    
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

void CameraController::HandleThirdPersonInput(float deltaTime) {
    // In third-person mode, we use arrow keys to rotate the camera around the player
    float velocity = m_MovementSpeed * deltaTime;
    float rotationSpeed = 100.0f * deltaTime; // Degrees per second
    
    // Arrow keys to rotate camera left/right
    if (m_Platform->IsKeyPressed(GLFW_KEY_LEFT)) {
        // Rotate camera left (increase yaw)
        float currentYaw = m_Camera->GetYaw();
        m_Camera->SetRotation(m_Camera->GetPitch(), currentYaw + rotationSpeed);
    }
    if (m_Platform->IsKeyPressed(GLFW_KEY_RIGHT)) {
        // Rotate camera right (decrease yaw)
        float currentYaw = m_Camera->GetYaw();
        m_Camera->SetRotation(m_Camera->GetPitch(), currentYaw - rotationSpeed);
    }
    
    // Arrow keys for camera up/down (optional)
    if (m_Platform->IsKeyPressed(GLFW_KEY_UP)) {
        // Rotate camera up (increase pitch, clamped to prevent over-rotation)
        float currentPitch = m_Camera->GetPitch();
        m_Camera->SetRotation(std::min(currentPitch + rotationSpeed, 89.0f), m_Camera->GetYaw());
    }
    if (m_Platform->IsKeyPressed(GLFW_KEY_DOWN)) {
        // Rotate camera down (decrease pitch, clamped to prevent over-rotation)
        float currentPitch = m_Camera->GetPitch();
        m_Camera->SetRotation(std::max(currentPitch - rotationSpeed, -89.0f), m_Camera->GetYaw());
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
    
    // Update the camera position
    SetTargetPosition(m_TargetPosition);
}

} // namespace Sylva 
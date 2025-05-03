#include "CameraController.h"
#include "../world/World.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

namespace Sylva {

CameraController::CameraController(Camera* camera, Platform* platform)
    : m_Camera(camera)
    , m_Platform(platform)
    , m_World(nullptr)
    , m_TargetPosition(glm::vec3(0.0f))
    , m_CurrentPosition(glm::vec3(0.0f, 0.0f, 5.0f))
    , m_OrbitDistance(5.0f)
    , m_VerticalOffset(2.0f)
    , m_SmoothingFactor(5.0f)
    , m_CollisionBuffer(0.2f)
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
    
    // Set initial rotation to look slightly downward (typical for third-person view)
    m_Camera->SetRotation(-30.0f, -180.0f);
}

void CameraController::SetTargetPosition(const glm::vec3& targetPosition) {
    m_TargetPosition = targetPosition;
    // Add vertical offset to look at player's head level rather than feet
    m_Camera->SetTarget(m_TargetPosition + glm::vec3(0.0f, m_VerticalOffset, 0.0f));
}

void CameraController::SetOrbitDistance(float distance) {
    m_OrbitDistance = std::max(0.1f, distance);
}

void CameraController::Update(float deltaTime) {
    // Process input for third-person mode
    HandleThirdPersonInput(deltaTime);
    
    // Handle mouse movement for camera rotation
    HandleMouseMovement();
    
    // Calculate desired camera position with collision handling
    glm::vec3 desiredPosition = CalculateCameraPosition();
    
    // Smoothly interpolate camera position
    m_CurrentPosition = glm::mix(m_CurrentPosition, desiredPosition, m_SmoothingFactor * deltaTime);
    
    // Set the camera position
    m_Camera->SetPosition(m_CurrentPosition);
    
    // Make camera look at the target (player) with vertical offset
    m_Camera->SetTarget(m_TargetPosition + glm::vec3(0.0f, m_VerticalOffset, 0.0f));
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
    
    // Arrow keys for camera up/down (pitch control)
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
    
    // Zoom controls using PageUp/PageDown
    if (m_Platform->IsKeyPressed(GLFW_KEY_PAGE_UP)) {
        // Zoom in
        SetOrbitDistance(m_OrbitDistance - (velocity * 2.0f));
    }
    if (m_Platform->IsKeyPressed(GLFW_KEY_PAGE_DOWN)) {
        // Zoom out
        SetOrbitDistance(m_OrbitDistance + (velocity * 2.0f));
    }
}

void CameraController::HandleMouseMovement() {
    // Get current mouse position
    double mouseX, mouseY;
    m_Platform->GetMousePosition(mouseX, mouseY);
    
    // Skip if it's the first mouse movement (avoid jumps)
    if (m_FirstMouse) {
        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;
        m_FirstMouse = false;
        return;
    }
    
    // Calculate offsets
    float xOffset = static_cast<float>(mouseX - m_LastMouseX);
    float yOffset = static_cast<float>(m_LastMouseY - mouseY); // Reversed since y-coordinates range from bottom to top
    
    // Update last position
    m_LastMouseX = mouseX;
    m_LastMouseY = mouseY;
    
    // Check if right mouse button is pressed (for orbit controls)
    if (m_Platform->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        // Apply sensitivity
        xOffset *= m_MouseSensitivity;
        yOffset *= m_MouseSensitivity;
        
        // Update camera rotation
        float currentYaw = m_Camera->GetYaw();
        float currentPitch = m_Camera->GetPitch();
        
        currentYaw += xOffset;
        currentPitch = std::max(-89.0f, std::min(currentPitch + yOffset, 89.0f));
        
        // Set the new rotation
        m_Camera->SetRotation(currentPitch, currentYaw);
    }
}

glm::vec3 CameraController::CalculateCameraPosition() {
    // Get camera orientation in radians
    float yawRadians = glm::radians(m_Camera->GetYaw());
    float pitchRadians = glm::radians(m_Camera->GetPitch());
    
    // Calculate offset based on orbit distance and camera orientation
    glm::vec3 offset;
    offset.x = -m_OrbitDistance * sin(yawRadians) * cos(pitchRadians);
    offset.y = m_OrbitDistance * sin(pitchRadians);
    offset.z = -m_OrbitDistance * cos(yawRadians) * cos(pitchRadians);
    
    // Calculate desired position (player position + vertical offset + rotated camera offset)
    glm::vec3 targetWithOffset = m_TargetPosition + glm::vec3(0.0f, m_VerticalOffset, 0.0f);
    glm::vec3 desiredPosition = targetWithOffset + offset;
    
    // Handle collision detection if we have a world reference
    glm::vec3 adjustedPosition = desiredPosition;
    if (m_World) {
        if (HandleCameraCollision(targetWithOffset, desiredPosition, adjustedPosition)) {
            // If collision occurred, use the adjusted position
            return adjustedPosition;
        }
    }
    
    // Return the desired position if no collision or no world reference
    return desiredPosition;
}

bool CameraController::HandleCameraCollision(const glm::vec3& from, const glm::vec3& to, glm::vec3& adjustedPosition) {
    if (!m_World) {
        // No collision possible without world reference
        return false;
    }
    
    // Direction from target to camera
    glm::vec3 direction = to - from;
    float distance = glm::length(direction);
    if (distance < 0.001f) return false; // Prevent division by zero
    
    // Normalize direction
    direction = direction / distance;
    
    // Perform simple collision check against terrain
    // Start a bit away from the player to prevent early collisions
    const float stepSize = 0.25f; // Check every 0.25 units along the ray
    const float startOffset = 0.5f; // Start checking 0.5 units away from player
    
    glm::vec3 currentPos = from + direction * startOffset;
    float currentDist = startOffset;
    
    while (currentDist < distance) {
        // Check for collision at current point
        float terrainHeight = m_World->GetTerrainHeightAt(currentPos.x, currentPos.z);
        
        // If the ray is below terrain, we have a collision
        if (currentPos.y < terrainHeight) {
            // Move back by buffer distance to avoid clipping
            adjustedPosition = currentPos - direction * m_CollisionBuffer;
            return true;
        }
        
        // Move to next point along ray
        currentPos += direction * stepSize;
        currentDist += stepSize;
    }
    
    // No collision detected
    return false;
}

} // namespace Sylva 
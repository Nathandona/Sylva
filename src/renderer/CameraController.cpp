#include "CameraController.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include "../core/Logger.h"

namespace Sylva {

CameraController::CameraController(Camera& camera, Platform& platform, InputManager& inputManager)
    : m_Camera(camera)
    , m_Platform(platform)
    , m_InputManager(&inputManager) // Store a pointer for backward compatibility with existing code
    , m_TargetPosition(glm::vec3(0.0f))
    , m_PlayerTarget(nullptr)
    , m_PlayerYaw(0.0f)
    , m_CurrentPosition(glm::vec3(0.0f, 2.0f, -5.0f))
    , m_OrbitDistance(DEFAULT_ORBIT_DISTANCE)
    , m_MinOrbitDistance(DEFAULT_MIN_ORBIT_DISTANCE)
    , m_MaxOrbitDistance(DEFAULT_MAX_ORBIT_DISTANCE)
    , m_VerticalOffset(DEFAULT_VERTICAL_OFFSET)
    , m_ShoulderOffset(0.0f) // Set to 0 for centered view in Cube World style
    , m_CameraPitch(FIXED_CAMERA_PITCH) // Use fixed pitch instead of default
    , m_CameraYawOffset(INITIAL_CAMERA_YAW)
    , m_FixedCameraMode(true) // Enable fixed camera mode by default
    , m_YawSmoothingFactor(DEFAULT_YAW_SMOOTHING)
    , m_PitchSmoothingFactor(DEFAULT_PITCH_SMOOTHING)
    , m_SmoothingFactor(DEFAULT_POSITION_SMOOTHING)
    , m_FirstMouse(true)
    , m_LastMouseX(0.0)
    , m_LastMouseY(0.0)
    , m_IsMouseOrbiting(false)
    , m_MovementSpeed(DEFAULT_MOVEMENT_SPEED)
    , m_MouseSensitivity(DEFAULT_MOUSE_SENSITIVITY)
    , m_ZoomSensitivity(DEFAULT_ZOOM_SENSITIVITY)
{
    // Initial camera setup
    
    double mouseX, mouseY;
    m_Platform.GetMousePosition(mouseX, mouseY);
    m_LastMouseX = mouseX;
    m_LastMouseY = mouseY;
    
    // Set initial rotation to fixed yaw in fixed camera mode
    if (m_FixedCameraMode) {
        m_Camera.SetRotation(m_CameraPitch, m_CameraYawOffset);
    } else {
        // Original behavior: set rotation based on player yaw + offset
        m_Camera.SetRotation(m_CameraPitch, m_PlayerYaw + m_CameraYawOffset);
    }
    
    // Set the mouse sensitivity to match the input manager
    m_MouseSensitivity = m_InputManager->GetMouseSensitivity();
}

void CameraController::SetTargetPosition(const glm::vec3& targetPosition) {
    m_TargetPosition = targetPosition;
}

void CameraController::SetPlayerTarget(Player* player) {
    m_PlayerTarget = player;
}

void CameraController::SetTargetYaw(float yaw) {
    m_PlayerYaw = yaw; // Updated variable name
}

void CameraController::SetOrbitDistance(float distance) {
    m_OrbitDistance = std::max(m_MinOrbitDistance, std::min(distance, m_MaxOrbitDistance));
}

void CameraController::Update(float deltaTime) {
    // Update target position from player if available
    if (m_PlayerTarget) {
        m_TargetPosition = m_PlayerTarget->GetPosition();
        m_PlayerYaw = m_PlayerTarget->GetYaw();
    }

    // Process input for camera controls using appropriate method
    if (m_InputManager) {
        HandleCameraInputWithManager(deltaTime);
        HandleMouseMovementWithManager();
    } else {
        HandleCameraInput(deltaTime);
        HandleMouseMovement();
    }
    
    // Apply smooth rotation
    static float lastYawOffset = m_CameraYawOffset;
    static float lastPitch = m_CameraPitch;
    
    // Smooth the rotation values
    lastYawOffset = glm::mix(lastYawOffset, m_CameraYawOffset, std::min(m_YawSmoothingFactor * deltaTime, 1.0f));
    lastPitch = glm::mix(lastPitch, m_CameraPitch, std::min(m_PitchSmoothingFactor * deltaTime, 1.0f));
    
    // Determine the final yaw for the camera this frame
    float effectiveYaw;
    
    if (m_FixedCameraMode) {
        // In fixed camera mode, use camera's own yaw independent of player
        effectiveYaw = m_CameraYawOffset;
        
        // Force fixed pitch in this mode
        lastPitch = FIXED_CAMERA_PITCH;
    } else {
        // Original behavior: link camera to player yaw + offset
        effectiveYaw = m_PlayerYaw + lastYawOffset;
    }
    
    // Apply rotation to the actual camera object
    m_Camera.SetRotation(lastPitch, effectiveYaw);
    
    // Calculate focus point with vertical offset
    glm::vec3 focusPoint = m_TargetPosition + glm::vec3(0.0f, m_VerticalOffset, 0.0f);
    
    // Add shoulder offset to focus point
    // In fixed camera mode, apply the shoulder offset based on camera orientation, not player
    float offsetYawRadians;
    
    if (m_FixedCameraMode) {
        offsetYawRadians = glm::radians(effectiveYaw + SHOULDER_OFFSET_ANGLE);
    } else {
        offsetYawRadians = glm::radians(m_PlayerYaw + SHOULDER_OFFSET_ANGLE);
    }
    
    glm::vec3 rightVector(sin(offsetYawRadians), 0.0f, cos(offsetYawRadians));
    focusPoint += rightVector * m_ShoulderOffset;
    
    // Calculate desired camera position
    glm::vec3 desiredPosition = CalculateCameraPosition();
    
    // Smoothly interpolate camera position
    m_CurrentPosition = glm::mix(m_CurrentPosition, desiredPosition, std::min(m_SmoothingFactor * deltaTime, 1.0f));
    
    // Set the camera position
    m_Camera.SetPosition(m_CurrentPosition);
}

void CameraController::OnWindowResize(int width, int height) {
    // Update camera's aspect ratio
    if (width > 0 && height > 0) {
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        m_Camera.SetAspectRatio(aspectRatio);
    }
}

void CameraController::HandleCameraInput(float deltaTime) {
    // Mouse wheel zooming
    float scrollOffset = m_Platform.GetMouseScrollOffset();
    if (scrollOffset != 0.0f) {
        // Scale the scroll sensitivity
        float zoomAmount = scrollOffset * m_ZoomSensitivity * ZOOM_SCROLL_MULTIPLIER;
        SetOrbitDistance(m_OrbitDistance - zoomAmount);
    }
    
    // PageUp/PageDown zooming (kept for backward compatibility)
    float keyZoomAmount = m_ZoomSensitivity * deltaTime * ZOOM_KEY_MULTIPLIER;
    if (m_Platform.IsKeyPressed(GLFW_KEY_PAGE_UP)) {
        SetOrbitDistance(m_OrbitDistance - keyZoomAmount);
    }
    if (m_Platform.IsKeyPressed(GLFW_KEY_PAGE_DOWN)) {
        SetOrbitDistance(m_OrbitDistance + keyZoomAmount);
    }
    
    // Camera left/right movement with Q and E keys
    float cameraYawSpeed = CAMERA_YAW_SPEED * deltaTime;
    if (m_Platform.IsKeyPressed(GLFW_KEY_Q)) {
        // Move camera left
        m_CameraYawOffset += cameraYawSpeed;
    }
    if (m_Platform.IsKeyPressed(GLFW_KEY_E)) {
        // Move camera right
        m_CameraYawOffset -= cameraYawSpeed;
    }
    
    // Arrow keys for pitch control (independent of mouse)
    float pitchSpeed = CAMERA_PITCH_SPEED * deltaTime;
    if (m_Platform.IsKeyPressed(GLFW_KEY_UP)) {
        m_CameraPitch = std::min(m_CameraPitch + pitchSpeed, MAX_PITCH);
    }
    if (m_Platform.IsKeyPressed(GLFW_KEY_DOWN)) {
        m_CameraPitch = std::max(m_CameraPitch - pitchSpeed, MIN_PITCH);
    }
    
    // Toggle shoulder offset with Tab key
    static bool tabPressed = false;
    if (m_Platform.IsKeyPressed(GLFW_KEY_TAB)) {
        if (!tabPressed) {
            ToggleShoulderSide();
            tabPressed = true;
        }
    } else {
        tabPressed = false;
    }
}

void CameraController::HandleMouseMovement() {
    // Get current mouse position
    double mouseX, mouseY;
    m_Platform.GetMousePosition(mouseX, mouseY);
    
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
    
    // Only process if RMB is held down
    if (m_Platform.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        if (!m_IsMouseOrbiting) {
            m_IsMouseOrbiting = true;
            // Avoid jump on first click by resetting first mouse
            m_FirstMouse = true;
            return;
        }
        
        // Apply sensitivity
        xOffset *= m_MouseSensitivity;
        yOffset *= m_MouseSensitivity;
        
        // Update pitch (vertical mouse movement)
        m_CameraPitch += yOffset;
        m_CameraPitch = std::max(MIN_PITCH, std::min(m_CameraPitch, MAX_PITCH)); // Clamp
        
        // Update yaw offset (horizontal mouse movement)
        // Subtract xOffset to make movement intuitive
        m_CameraYawOffset -= xOffset;
    } 
    else {
        if (m_IsMouseOrbiting) {
            m_IsMouseOrbiting = false;
            m_FirstMouse = true;
        }
    }
}

void CameraController::HandleCameraInputWithManager(float deltaTime) {
    // Mouse wheel zooming
    float scrollOffset = m_InputManager->GetScrollDelta();
    if (scrollOffset != 0.0f) {
        // Scale the scroll sensitivity
        float zoomAmount = scrollOffset * m_ZoomSensitivity * ZOOM_SCROLL_MULTIPLIER;
        SetOrbitDistance(m_OrbitDistance - zoomAmount);
    }
    
    // Zoom with action keys
    float keyZoomAmount = m_ZoomSensitivity * deltaTime * ZOOM_KEY_MULTIPLIER;
    if (m_InputManager->IsActionPressed("ZoomIn")) {
        SetOrbitDistance(m_OrbitDistance - keyZoomAmount);
    }
    if (m_InputManager->IsActionPressed("ZoomOut")) {
        SetOrbitDistance(m_OrbitDistance + keyZoomAmount);
    }
    
    // Skip camera rotation inputs in fixed camera mode
    if (m_FixedCameraMode) {
        // In future, could implement manual camera rotation here (Q/E keys)
        return;
    }
    
    // Original camera rotation behavior (only used when not in fixed mode)
    float cameraYawSpeed = CAMERA_YAW_SPEED * deltaTime;
    if (m_InputManager->IsActionPressed("TurnLeft")) {
        // Move camera left
        m_CameraYawOffset += cameraYawSpeed;
    }
    if (m_InputManager->IsActionPressed("TurnRight")) {
        // Move camera right
        m_CameraYawOffset -= cameraYawSpeed;
    }
    
    // Camera pitch control (Up/Down keys or equivalent)
    float pitchSpeed = CAMERA_PITCH_SPEED * deltaTime;
    if (m_InputManager->IsActionPressed("LookUp")) {
        m_CameraPitch = std::min(m_CameraPitch + pitchSpeed, MAX_PITCH);
    }
    if (m_InputManager->IsActionPressed("LookDown")) {
        m_CameraPitch = std::max(m_CameraPitch - pitchSpeed, MIN_PITCH);
    }
}

void CameraController::HandleMouseMovementWithManager() {
    // Skip mouse movement handling in fixed camera mode
    if (m_FixedCameraMode) {
        return;
    }
    
    // Get mouse delta from input manager
    float deltaX = 0.0f, deltaY = 0.0f;
    m_InputManager->GetMouseDelta(deltaX, deltaY);
    
    // Only process if RMB is held down
    if (m_InputManager->IsRightMouseButtonPressed()) {
        if (!m_IsMouseOrbiting) {
            m_IsMouseOrbiting = true;
            return;
        }
        
        // Apply sensitivity
        deltaX *= m_MouseSensitivity;
        deltaY *= m_MouseSensitivity;
        
        // Update pitch (vertical mouse movement)
        m_CameraPitch += deltaY;
        m_CameraPitch = std::max(MIN_PITCH, std::min(m_CameraPitch, MAX_PITCH)); // Clamp
        
        // Update yaw offset (horizontal mouse movement)
        m_CameraYawOffset -= deltaX;
    } 
    else {
        if (m_IsMouseOrbiting) {
            m_IsMouseOrbiting = false;
        }
    }
}

glm::vec3 CameraController::CalculateCameraPosition() {
    // Convert player's yaw to radians (this is the direction the player is facing)
    float playerYawRad = glm::radians(m_PlayerYaw);
    
    // Add the yaw offset from mouse control
    float totalYawRad = playerYawRad + glm::radians(m_CameraYawOffset);
    
    // Calculate pitch in radians
    float pitchRad = glm::radians(m_CameraPitch);
    
    // Define the focus point (what the camera will look at) with shoulder offset
    glm::vec3 focusPoint = m_TargetPosition + glm::vec3(0.0f, m_VerticalOffset, 0.0f);
    
    // Add shoulder offset to focus point
    float shoulderYawRad = playerYawRad + glm::radians(SHOULDER_OFFSET_ANGLE); // Right is 90 degrees from forward
    glm::vec3 rightVector(sin(shoulderYawRad), 0.0f, cos(shoulderYawRad));
    focusPoint += rightVector * m_ShoulderOffset;
    
    // Calculate camera position using spherical coordinates
    // Important: For a third-person camera behind the player, we need to reverse the direction
    // When yaw=0, player faces +Z, so camera should be at -Z (180 degrees from facing direction)
    float cameraYawRad = totalYawRad + glm::radians(CAMERA_BEHIND_ANGLE);
    
    // Convert from spherical to Cartesian coordinates
    float x = m_OrbitDistance * cos(pitchRad) * sin(cameraYawRad);
    float y = m_OrbitDistance * -sin(pitchRad); // Negate for correct orientation
    float z = m_OrbitDistance * cos(pitchRad) * cos(cameraYawRad);
    
    // The offset vector FROM the focus point TO the camera
    glm::vec3 cameraOffset(x, y, z);
    
    // Calculate final camera position
    glm::vec3 desiredPosition = focusPoint + cameraOffset;
    
    return desiredPosition;
}

} // namespace Sylva 
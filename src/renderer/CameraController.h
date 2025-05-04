#pragma once

#include "Camera.h"
#include "../platform/Platform.h"
#include "../world/Player.h"
#include <algorithm>

namespace Sylva {

class CameraController {
public:
    CameraController(Camera* camera, Platform* platform);
    ~CameraController() = default;
    
    // For third-person/orbit mode
    void SetTargetPosition(const glm::vec3& targetPosition);
    
    // Set player target (Cube World style camera)
    void SetPlayerTarget(Player* player);
    Player* GetPlayerTarget() const { return m_PlayerTarget; }
    
    void SetOrbitDistance(float distance);
    float GetOrbitDistance() const { return m_OrbitDistance; }
    
    // Set min/max orbit distance for zoom constraints
    void SetMinOrbitDistance(float distance) { m_MinOrbitDistance = std::max(0.1f, distance); }
    void SetMaxOrbitDistance(float distance) { m_MaxOrbitDistance = std::max(m_MinOrbitDistance, distance); }
    float GetMinOrbitDistance() const { return m_MinOrbitDistance; }
    float GetMaxOrbitDistance() const { return m_MaxOrbitDistance; }
    
    // Set the target yaw (player's rotation)
    void SetTargetYaw(float yaw);
    
    // Update the camera based on platform input
    void Update(float deltaTime);
    
    // For handling window resize
    void OnWindowResize(int width, int height);
    
    // Get the camera
    Camera* GetCamera() const { return m_Camera; }
    
    // Get/Set smoothing factor
    float GetSmoothingFactor() const { return m_SmoothingFactor; }
    void SetSmoothingFactor(float factor) { m_SmoothingFactor = std::max(0.0f, std::min(factor, 1.0f)); }
    
    // Get/Set vertical offset
    float GetVerticalOffset() const { return m_VerticalOffset; }
    void SetVerticalOffset(float offset) { m_VerticalOffset = offset; }
    
    // Get/Set shoulder offset
    float GetShoulderOffset() const { return m_ShoulderOffset; }
    void SetShoulderOffset(float offset) { m_ShoulderOffset = offset; }
    
    // Toggle shoulder side (left/right)
    void ToggleShoulderSide() { m_ShoulderOffset = -m_ShoulderOffset; }
    
private:
    // Handle input based on the current control mode
    void HandleCameraInput(float deltaTime);
    
    // Handle mouse movement for camera rotation
    void HandleMouseMovement();
    
    // Calculate desired camera position with collision handling
    glm::vec3 CalculateCameraPosition();
    
    Camera* m_Camera;
    Platform* m_Platform;
    
    // Target tracking
    glm::vec3 m_TargetPosition;
    Player* m_PlayerTarget;  // Pointer to the player (for Cube World style camera)
    float m_PlayerYaw;  // Renamed from m_TargetYaw for clarity
    
    // Camera position and state
    glm::vec3 m_CurrentPosition;
    float m_OrbitDistance;
    float m_MinOrbitDistance;  // Minimum zoom distance
    float m_MaxOrbitDistance;  // Maximum zoom distance
    float m_VerticalOffset;
    float m_ShoulderOffset;  // Horizontal offset from center (positive = right, negative = left)
    
    // Camera orientation state (controlled internally/by mouse)
    float m_CameraPitch;    // Renamed from m_CurrentPitch for clarity
    float m_CameraYawOffset; // Renamed from m_CurrentYawOffset for clarity
    
    // Rotation smoothing
    float m_YawSmoothingFactor;
    float m_PitchSmoothingFactor;
    
    // Position smoothing
    float m_SmoothingFactor;
    
    // Mouse state
    bool m_FirstMouse;
    double m_LastMouseX;
    double m_LastMouseY;
    bool m_IsMouseOrbiting;
    
    // Movement settings
    float m_MovementSpeed;
    float m_MouseSensitivity;
    float m_ZoomSensitivity;
};

} // namespace Sylva 
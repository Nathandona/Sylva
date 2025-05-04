#pragma once

#include "Camera.h"
#include "../platform/Platform.h"
#include "../world/Terrain.h"
#include <algorithm>

namespace Sylva {

class World;

class CameraController {
public:
    CameraController(Camera* camera, Platform* platform);
    ~CameraController() = default;
    
    // For third-person/orbit mode
    void SetTargetPosition(const glm::vec3& targetPosition);
    void SetOrbitDistance(float distance);
    float GetOrbitDistance() const { return m_OrbitDistance; }
    
    // Set the target yaw (player's rotation)
    void SetTargetYaw(float yaw);
    
    // Update the camera based on platform input
    void Update(float deltaTime);
    
    // For handling window resize
    void OnWindowResize(int width, int height);
    
    // Get the camera
    Camera* GetCamera() const { return m_Camera; }
    
    // Set the world reference for collision detection
    void SetWorld(World* world) { m_World = world; }
    
    // Get/Set smoothing factor
    float GetSmoothingFactor() const { return m_SmoothingFactor; }
    void SetSmoothingFactor(float factor) { m_SmoothingFactor = std::max(0.0f, std::min(factor, 1.0f)); }
    
    // Get/Set vertical offset
    float GetVerticalOffset() const { return m_VerticalOffset; }
    void SetVerticalOffset(float offset) { m_VerticalOffset = offset; }
    
    // Get/Set collision buffer distance
    float GetCollisionBuffer() const { return m_CollisionBuffer; }
    void SetCollisionBuffer(float buffer) { m_CollisionBuffer = buffer; }
    
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
    
    // Check for collisions with terrain and adjust camera position
    bool HandleCameraCollision(const glm::vec3& from, const glm::vec3& to, glm::vec3& adjustedPosition);
    
    // Improved sphere cast collision for camera
    bool SphereCastCollision(const glm::vec3& from, const glm::vec3& to, float radius, glm::vec3& hitPosition);
    
    Camera* m_Camera;
    Platform* m_Platform;
    World* m_World;
    
    // Target tracking
    glm::vec3 m_TargetPosition;
    float m_PlayerYaw;  // Renamed from m_TargetYaw for clarity
    
    // Camera position and state
    glm::vec3 m_CurrentPosition;
    float m_OrbitDistance;
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
    
    // Collision handling
    float m_CollisionBuffer;
    float m_CollisionRadius;  // For sphere cast collision
    
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
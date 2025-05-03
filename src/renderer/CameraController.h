#pragma once

#include "Camera.h"
#include "../platform/Platform.h"
#include "../world/Terrain.h"
#include <algorithm>

namespace Sylva {

class World;

class CameraController {
public:
    enum class ControlMode {
        ThirdPerson
    };

    CameraController(Camera* camera, Platform* platform);
    ~CameraController() = default;
    
    // For third-person/orbit mode
    void SetTargetPosition(const glm::vec3& targetPosition);
    void SetOrbitDistance(float distance);
    
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
    
private:
    // Handle input based on the current control mode
    void HandleThirdPersonInput(float deltaTime);
    
    // Handle mouse movement for camera rotation
    void HandleMouseMovement();
    
    // Calculate desired camera position with collision handling
    glm::vec3 CalculateCameraPosition();
    
    // Check for collisions with terrain and adjust camera position
    bool HandleCameraCollision(const glm::vec3& from, const glm::vec3& to, glm::vec3& adjustedPosition);
    
    Camera* m_Camera;
    Platform* m_Platform;
    World* m_World;
    
    // For third-person/orbit mode
    glm::vec3 m_TargetPosition;
    glm::vec3 m_CurrentPosition;
    float m_OrbitDistance;
    float m_VerticalOffset;
    
    // Camera smoothing
    float m_SmoothingFactor;
    
    // Collision handling
    float m_CollisionBuffer;
    
    // Mouse state
    bool m_FirstMouse;
    double m_LastMouseX;
    double m_LastMouseY;
    
    // Movement settings
    float m_MovementSpeed;
    float m_MouseSensitivity;
};

} // namespace Sylva 
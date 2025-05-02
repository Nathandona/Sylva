#pragma once

#include "Camera.h"
#include "../platform/Platform.h"

namespace Sylva {

class CameraController {
public:
    enum class ControlMode {
        FirstPerson,
        ThirdPerson,
        Orbit
    };

    CameraController(Camera* camera, Platform* platform);
    ~CameraController() = default;

    // Set the control mode
    void SetControlMode(ControlMode mode);
    
    // For third-person/orbit mode
    void SetTargetPosition(const glm::vec3& targetPosition);
    void SetOrbitDistance(float distance);
    
    // Update the camera based on platform input
    void Update(float deltaTime);
    
    // For handling window resize
    void OnWindowResize(int width, int height);
    
    // Get the camera
    Camera* GetCamera() const { return m_Camera; }
    
private:
    // Handle input based on the current control mode
    void HandleFirstPersonInput(float deltaTime);
    void HandleThirdPersonInput(float deltaTime);
    void HandleOrbitInput(float deltaTime);
    
    // Mouse movement
    void HandleMouseMovement();
    
    Camera* m_Camera;
    Platform* m_Platform;
    
    ControlMode m_ControlMode;
    
    // For third-person/orbit mode
    glm::vec3 m_TargetPosition;
    float m_OrbitDistance;
    
    // Mouse state
    bool m_FirstMouse;
    double m_LastMouseX;
    double m_LastMouseY;
    
    // Movement settings
    float m_MovementSpeed;
    float m_MouseSensitivity;
};

} // namespace Sylva 
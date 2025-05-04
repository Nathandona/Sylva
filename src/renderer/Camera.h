#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Sylva {

class Camera {
public:
    enum class ProjectionType {
        Perspective,
        Orthographic
    };

    Camera(float fov = 45.0f, float aspectRatio = 16.0f / 9.0f, float nearClip = 0.1f, float farClip = 100.0f);
    ~Camera() = default;

    // Set camera properties
    void SetPosition(const glm::vec3& position);
    void SetRotation(float pitch, float yaw);
    void SetTarget(const glm::vec3& target); // Look at a specific point
    void SetAspectRatio(float aspectRatio);
    void SetProjectionType(ProjectionType type);
    void SetPerspectiveProjection(float fov, float aspectRatio, float nearClip, float farClip);
    void SetOrthographicProjection(float left, float right, float bottom, float top, float nearClip, float farClip);

    // Get camera properties
    glm::vec3 GetPosition() const { return m_Position; }
    glm::vec3 GetFront() const { return m_Front; }
    glm::vec3 GetRight() const { return m_Right; }
    glm::vec3 GetUp() const { return m_Up; }
    float GetYaw() const { return m_Yaw; }
    float GetPitch() const { return m_Pitch; }

    // Get view and projection matrices
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

    // Update camera vectors based on rotation
    void UpdateCameraVectors();

private:
    // Camera position and orientation
    glm::vec3 m_Position;
    glm::vec3 m_Front;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    // Euler angles (in degrees)
    float m_Yaw;
    float m_Pitch;

    // Removed: Camera options (movement speed, mouse sensitivity)
    float m_Zoom; // Field of view (kept for projection calculation)

    // Projection settings
    ProjectionType m_ProjectionType;
    float m_FOV;
    float m_AspectRatio;
    float m_NearClip;
    float m_FarClip;

    // For orthographic projection
    float m_OrthoLeft;
    float m_OrthoRight;
    float m_OrthoBottom;
    float m_OrthoTop;
};

} // namespace Sylva 
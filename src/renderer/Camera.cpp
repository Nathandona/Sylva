#include "Camera.h"
#include <algorithm>
#include <iostream>

namespace Sylva {

// Default camera values
constexpr float YAW = -90.0f;
constexpr float PITCH = 0.0f;
constexpr float SPEED = 2.5f;
constexpr float SENSITIVITY = 0.1f;
constexpr float ZOOM = 60.0f;

Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
    : m_Position(glm::vec3(0.0f, 0.0f, 3.0f))
    , m_Front(glm::vec3(0.0f, 0.0f, -1.0f))
    , m_Up(glm::vec3(0.0f, 1.0f, 0.0f))
    , m_Right(glm::vec3(1.0f, 0.0f, 0.0f))
    , m_WorldUp(glm::vec3(0.0f, 1.0f, 0.0f))
    , m_Yaw(YAW)
    , m_Pitch(PITCH)
    , m_MovementSpeed(SPEED)
    , m_MouseSensitivity(SENSITIVITY)
    , m_Zoom(ZOOM)
    , m_ProjectionType(ProjectionType::Perspective)
    , m_FOV(fov)
    , m_AspectRatio(aspectRatio)
    , m_NearClip(nearClip)
    , m_FarClip(farClip)
    , m_OrthoLeft(-1.0f)
    , m_OrthoRight(1.0f)
    , m_OrthoBottom(-1.0f)
    , m_OrthoTop(1.0f)
{
    UpdateCameraVectors();
}

void Camera::SetPosition(const glm::vec3& position) {
    m_Position = position;
}

void Camera::SetRotation(float pitch, float yaw) {
    m_Pitch = pitch;
    m_Yaw = yaw;
    UpdateCameraVectors();
}

void Camera::SetTarget(const glm::vec3& target) {
    // Look at a specific point
    if (target == m_Position) {
        // Can't look at own position
        return;
    }
    
    glm::vec3 direction = glm::normalize(target - m_Position);
    m_Pitch = glm::degrees(asin(direction.y));
    m_Yaw = glm::degrees(atan2(direction.z, direction.x)) + 90.0f;
    
    UpdateCameraVectors();
}

void Camera::SetAspectRatio(float aspectRatio) {
    m_AspectRatio = aspectRatio;
}

void Camera::SetProjectionType(ProjectionType type) {
    m_ProjectionType = type;
}

void Camera::SetPerspectiveProjection(float fov, float aspectRatio, float nearClip, float farClip) {
    m_ProjectionType = ProjectionType::Perspective;
    m_FOV = fov;
    m_AspectRatio = aspectRatio;
    m_NearClip = nearClip;
    m_FarClip = farClip;
}

void Camera::SetOrthographicProjection(float left, float right, float bottom, float top, float nearClip, float farClip) {
    m_ProjectionType = ProjectionType::Orthographic;
    m_OrthoLeft = left;
    m_OrthoRight = right;
    m_OrthoBottom = bottom;
    m_OrthoTop = top;
    m_NearClip = nearClip;
    m_FarClip = farClip;
}

void Camera::MoveForward(float distance) {
    m_Position += m_Front * distance;
}

void Camera::MoveRight(float distance) {
    m_Position += m_Right * distance;
}

void Camera::MoveUp(float distance) {
    m_Position += m_WorldUp * distance;
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
    xOffset *= m_MouseSensitivity;
    yOffset *= m_MouseSensitivity;

    m_Yaw += xOffset;
    m_Pitch += yOffset;

    // Constrain pitch to avoid flipping
    if (constrainPitch) {
        m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);
    }

    // Update camera vectors
    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yOffset) {
    // Adjust FOV based on scroll (zoom)
    m_Zoom -= yOffset;
    m_Zoom = std::clamp(m_Zoom, 20.0f, 70.0f);
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

glm::mat4 Camera::GetProjectionMatrix() const {
    if (m_ProjectionType == ProjectionType::Perspective) {
        return glm::perspective(glm::radians(m_Zoom), m_AspectRatio, m_NearClip, m_FarClip);
    } else {
        return glm::ortho(m_OrthoLeft, m_OrthoRight, m_OrthoBottom, m_OrthoTop, m_NearClip, m_FarClip);
    }
}

void Camera::UpdateCameraVectors() {
    // Calculate the new front vector
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    
    m_Front = glm::normalize(front);
    
    // Recalculate the right and up vectors
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up    = glm::normalize(glm::cross(m_Right, m_Front));
}

} // namespace Sylva 
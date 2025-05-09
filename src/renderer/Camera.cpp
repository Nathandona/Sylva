#include "camera.h"
#include "core/logger.h"
#include "core/config.h"
#include "world/player.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>

namespace Sylva {

Camera::Camera() : m_yaw(0.0f), m_pitch(0.0f) {
    // Load camera parameters from config if available
    m_params.orbitDistance = Config::getFloat("Camera.orbit_distance", m_params.orbitDistance);
    m_params.minDistance = Config::getFloat("Camera.min_distance", m_params.minDistance);
    m_params.maxDistance = Config::getFloat("Camera.max_distance", m_params.maxDistance);
    m_params.targetHeight = Config::getFloat("Camera.target_height", m_params.targetHeight);
    m_params.rotationSpeed = Config::getFloat("Camera.rotation_speed", m_params.rotationSpeed);
    m_params.zoomSpeed = Config::getFloat("Camera.zoom_speed", m_params.zoomSpeed);
    
    // Initialize vectors
    m_forward = Vec3(0.0f, 0.0f, -1.0f);
    m_up = Vec3(0.0f, 1.0f, 0.0f);
    m_right = Vec3(1.0f, 0.0f, 0.0f);
    
    // Set initial position
    m_position = Vec3(0.0f, 5.0f, 5.0f);
    m_target = Vec3(0.0f, 0.0f, 0.0f);
    
    Logger::logDebug("Camera initialized with orbit distance: " + 
                    std::to_string(m_params.orbitDistance));
}

Camera::Camera(const CameraParams& params) : m_params(params), m_yaw(0.0f), m_pitch(0.0f) {
    // Initialize vectors
    m_forward = Vec3(0.0f, 0.0f, -1.0f);
    m_up = Vec3(0.0f, 1.0f, 0.0f);
    m_right = Vec3(1.0f, 0.0f, 0.0f);
    
    // Set initial position
    m_position = Vec3(0.0f, 5.0f, 5.0f);
    m_target = Vec3(0.0f, 0.0f, 0.0f);
    
    Logger::logDebug("Camera initialized with custom parameters");
}

void Camera::updateRotation(const InputState& input) {
    // Get the mouse sensitivity from config if available
    float mouseSensitivity = Config::getFloat("Input.mouse_sensitivity", 0.1f);
    
    // Process mouse rotation - Cube World style camera movement
    // Scale by sensitivity and make rotation speed consistent regardless of frame rate
    m_yaw += input.mouseDeltaX * mouseSensitivity * 0.01f;
    m_pitch += input.mouseDeltaY * mouseSensitivity * 0.01f;
    
    // Clamp pitch to avoid flipping (limit to slightly less than 90 degrees up/down)
    m_pitch = std::clamp(m_pitch, -1.5f, 1.5f); // About 85 degrees
}

void Camera::updateZoom(const InputState& input) {
    // Process zoom using both mouse wheel and right mouse button
    // Mouse wheel zooming
    m_params.orbitDistance -= input.mouseWheelDelta * m_params.zoomSpeed;
    
    // Right mouse button + vertical mouse movement for zooming
    if (input.mouseRightButton) {
        m_params.orbitDistance -= input.mouseDeltaY * m_params.zoomSpeed * 0.05f;
    }
    
    // Clamp orbit distance
    m_params.orbitDistance = std::clamp(m_params.orbitDistance, 
                                       m_params.minDistance, 
                                       m_params.maxDistance);
}

void Camera::updatePosition() {
    // Calculate camera position based on orbit parameters (spherical coordinates)
    float x = m_params.orbitDistance * std::sin(m_yaw) * std::cos(m_pitch);
    float y = m_params.orbitDistance * std::sin(m_pitch);
    float z = m_params.orbitDistance * std::cos(m_yaw) * std::cos(m_pitch);
    
    // Set camera position relative to target
    m_position = m_target + Vec3(x, y, z);
}

void Camera::updateVectors() {
    // Update camera vectors
    m_forward = glm::normalize(m_target - m_position);
    m_right = glm::normalize(glm::cross(m_forward, Vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}

void Camera::updateOrbit(float deltaTime, const Player& player, const InputState& input) {
    // Get player position and update target
    Vec3 playerPos = player.getPosition();
    m_target = playerPos + Vec3(0.0f, m_params.targetHeight, 0.0f);
    
    // Update camera components
    updateRotation(input);
    updateZoom(input);
    updatePosition();
    updateVectors();
}

void Camera::setTargetHeight(float height) {
    m_params.targetHeight = height;
}

void Camera::setDistance(float distance) {
    m_params.orbitDistance = std::clamp(distance, m_params.minDistance, m_params.maxDistance);
}

void Camera::setParams(const CameraParams& params) {
    m_params = params;
}

Vec3 Camera::getPosition() const {
    return m_position;
}

Vec3 Camera::getForward() const {
    return m_forward;
}

Vec3 Camera::getUp() const {
    return m_up;
}

Vec3 Camera::getRight() const {
    return m_right;
}

Mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_target, Vec3(0.0f, 1.0f, 0.0f));
}

Mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);
}

} // namespace Sylva 
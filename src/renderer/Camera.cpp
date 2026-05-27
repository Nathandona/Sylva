#include "camera.h"
#include "core/logger.h"
#include "core/config.h"
#include "world/block.h"
#include "world/player.h"
#include "world/voxel_world.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>

namespace Sylva {

Camera::Camera() {
    // Defaults come from NSDMI; only override the params from config here.
    m_params.orbitDistance = Config::getFloat("Camera.orbit_distance", m_params.orbitDistance);
    m_params.minDistance = Config::getFloat("Camera.min_distance", m_params.minDistance);
    m_params.maxDistance = Config::getFloat("Camera.max_distance", m_params.maxDistance);
    m_params.targetHeight = Config::getFloat("Camera.target_height", m_params.targetHeight);
    m_params.rotationSpeed = Config::getFloat("Camera.rotation_speed", m_params.rotationSpeed);
    m_params.zoomSpeed = Config::getFloat("Camera.zoom_speed", m_params.zoomSpeed);
    Logger::logDebug("Camera initialized with orbit distance: " + std::to_string(m_params.orbitDistance));
}

Camera::Camera(const CameraParams& params) : m_params(params) {
    Logger::logDebug("Camera initialized with custom parameters");
}

void Camera::updateRotation(const InputState& input) {
    // Mouse delta is the sum of pixel motion since the previous frame (the
    // platform callback accumulates), so framerate cancels out: total yaw per
    // physical mouse movement is identical at 60Hz and 144Hz.
    const float mouseSensitivity = Config::getFloat("Input.mouse_sensitivity", 0.1f);
    m_yaw += input.mouseDeltaX * mouseSensitivity * 0.01f;
    m_pitch += input.mouseDeltaY * mouseSensitivity * 0.01f;

    // Clamp pitch tighter than ±90° — anywhere past ~60° starts feeling
    // disorienting in third-person and makes the look-up/look-down workflow
    // unpleasant.
    m_pitch = std::clamp(m_pitch, -1.05f, 1.0f); // ~ -60° .. +57°
}

void Camera::updateZoom(const InputState& input) {
    // Mouse wheel only. Previous build also zoomed on right-mouse-drag, which
    // collided with mouse-look and triggered by accident.
    m_params.orbitDistance -= input.mouseWheelDelta * m_params.zoomSpeed;
    m_params.orbitDistance = std::clamp(m_params.orbitDistance, m_params.minDistance, m_params.maxDistance);
}

void Camera::updatePosition() {
    // Calculate camera position based on orbit parameters (spherical coordinates)
    float const x = m_params.orbitDistance * std::sin(m_yaw) * std::cos(m_pitch);
    float const y = m_params.orbitDistance * std::sin(m_pitch);
    float const z = m_params.orbitDistance * std::cos(m_yaw) * std::cos(m_pitch);

    // Set camera position relative to target
    m_position = m_target + Vec3(x, y, z);
}

void Camera::updateVectors() {
    // Update camera vectors
    m_forward = glm::normalize(m_target - m_position);
    m_right = glm::normalize(glm::cross(m_forward, Vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}

void Camera::updateOrbit(float deltaTime, const Player& player, const InputState& input, const VoxelWorld& world) {
    // Smooth the camera target toward the player's head position. Snapping
    // (the previous behavior) caused visible jitter on jumps and rapid
    // movement; exponential damping keeps the camera responsive while
    // absorbing single-frame spikes. 1 - exp(-dt*k) is framerate-independent.
    const Vec3 desiredTarget = player.getPosition() + Vec3(0.0f, m_params.targetHeight, 0.0f);
    const float k = Config::getFloat("Camera.follow_smoothing", 12.0f);
    const float t = 1.0f - std::exp(-deltaTime * k);
    m_target = glm::mix(m_target, desiredTarget, t);

    updateRotation(input);
    updateZoom(input);
    updatePosition();

    // Camera collision: march from target toward the freshly-computed
    // orbit position. If we hit an opaque solid (anything that isn't AIR /
    // WATER / LEAVES), pull the camera in just shy of the block face so
    // the view doesn't get buried inside terrain or trunks.
    const Vec3 toCam = m_position - m_target;
    const float orbitDist = glm::length(toCam);
    if (orbitDist > 0.0001f) {
        const Vec3 dir = toCam / orbitDist;
        constexpr float kStep = 0.1f;        // world-y; finer than cellSize=0.25 for accuracy
        constexpr float kFaceMargin = 0.15f; // pull back so we don't clip into the face
        constexpr float kMinDist = 0.5f;     // never collapse fully onto the player
        for (float s = kStep; s < orbitDist; s += kStep) {
            const Vec3 sample = m_target + dir * s;
            const BlockType b = world.getBlockAt(sample);
            // Transparent blocks (leaves, water, air) don't occlude — only
            // opaque solids pull the camera in.
            if (b != BlockType::AIR && BlockData::isSolid(b) && !BlockData::isTransparent(b)) {
                const float clamped = std::max(kMinDist, s - kFaceMargin);
                m_position = m_target + dir * clamped;
                break;
            }
        }
    }

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
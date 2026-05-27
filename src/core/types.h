#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace Sylva {

/**
 * @brief Common type definitions for the Sylva engine
 */

// Basic vector types using GLM
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;

// Matrix types
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

// Integer vector types
using Vec2i = glm::ivec2;
using Vec3i = glm::ivec3;
using Vec4i = glm::ivec4;

/**
 * @brief Camera parameters
 */
struct CameraParams {
    float orbitDistance = 5.0f; // Distance from target
    float minDistance = 2.0f;   // Minimum zoom distance
    float maxDistance = 10.0f;  // Maximum zoom distance
    float targetHeight = 1.7f;  // Height of look-at point
    float rotationSpeed = 1.0f; // Mouse rotation sensitivity
    float zoomSpeed = 1.0f;     // Zoom sensitivity
};

/**
 * @brief Player parameters
 */
struct PlayerParams {
    float moveSpeed = 5.0f;       // Units per second
    float rotationSpeed = 2.0f;   // Radians per second
    float collisionRadius = 0.8f; // Increased collision radius to match larger player size
    float height = 2.4f;          // Increased player height for visual contrast with micro-voxels
    float jumpHeight = 3.0f;      // Maximum jump height
    float jumpForce = 8.0f;       // Initial jump velocity
    float gravity = 15.0f;        // Gravity acceleration
    bool isGrounded = true;       // Whether the player is on the ground
    float autoStepHeight = 0.30f; // Max vertical step the player climbs automatically (world-y; ~1 voxel at cellSize=0.25)
    // Cuboid representation with larger blocks
    float width = 1.6f;                    // Increased width for visual contrast with micro-voxels
    float depth = 1.6f;                    // Increased depth for visual contrast with micro-voxels
    Vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // White color (RGBA)
};

/**
 * @brief World parameters
 */
struct WorldParams {
    int terrainSize = 100;   // Size of terrain grid
    float cellSize = 0.25f;  // Size of each terrain cell (reduced for micro-voxels)
    float maxHeight = 10.0f; // Maximum terrain height
    int seed = 0;            // Terrain generation seed

    // Micro-voxel specific parameters
    float noiseScale = 0.02f;       // Scale for base terrain noise
    float detailScale = 0.08f;      // Scale for detail noise
    float terrainSmoothness = 0.6f; // Terrain smoothness (0.0-1.0, higher = smoother)

    // Terrain is sampled on a (plateauWidthVoxels x plateauWidthVoxels) grid in
    // XZ — every voxel column inside the same cell gets the same height. This
    // turns steep noise gradients into walkable stair-step terrain. Set this
    // >= player width (in voxels) so each step has room for the player bbox.
    int plateauWidthVoxels = 4;
};

/**
 * @brief UI parameters
 */
struct UIParams {
    float crosshairSize = 20.0f;                    // Size in pixels
    Vec4 crosshairColor = {1.0f, 1.0f, 1.0f, 1.0f}; // RGBA
    float crosshairThickness = 2.0f;                // Thickness in pixels
};

/**
 * @brief Audio parameters
 */
struct AudioParams {
    float masterVolume = 0.8f;  // Overall volume (0.0 to 1.0)
    float musicVolume = 0.7f;   // Music volume multiplier
    float sfxVolume = 1.0f;     // Sound effects volume multiplier
    float ambientVolume = 0.5f; // Ambient sounds volume multiplier
    float voiceVolume = 1.0f;   // Voice/dialogue volume multiplier
    float distanceModel = 1.0f; // Rolloff factor for distance attenuation
    int maxSources = 32;        // Maximum number of simultaneous sound sources
};

/**
 * @brief Input state for the engine
 */
struct InputState {
    // Movement
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;

    // Camera control
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    float mouseWheelDelta = 0.0f; // Mouse wheel movement for zooming
    bool mouseLeftButton = false;
    bool mouseRightButton = false;
    bool toggleCursor = false; // Toggle cursor visibility/lock

    // Actions
    bool jump = false;
    bool interact = false;
    bool sprint = false;

    // Audio controls
    bool volumeUp = false;
    bool volumeDown = false;
    bool mute = false;
};

} // namespace Sylva
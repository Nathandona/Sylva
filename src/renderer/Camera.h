#pragma once

#include "core/types.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Sylva {

// Forward declarations
class Player;
struct InputState;

/**
 * @brief Third-person orbit camera system
 * 
 * A camera that orbits around the player character at a fixed distance,
 * always looking at the player. Supports zooming in/out and rotation.
 */
class Camera {
public:
    /**
     * @brief Constructor
     */
    Camera();

    /**
     * @brief Constructor with parameters
     * @param params Camera parameters to use
     */
    Camera(const CameraParams& params);

    /**
     * @brief Update the camera based on player position and input
     * @param deltaTime Time since last frame
     * @param player The player object to track
     * @param input Current input state
     */
    void updateOrbit(float deltaTime, const Player& player, const InputState& input);

    /**
     * @brief Set the target height (the height at which the camera looks at the player)
     * @param height The new target height
     */
    void setTargetHeight(float height);

    /**
     * @brief Set the distance from the camera to the player
     * @param distance The new distance
     */
    void setDistance(float distance);

    /**
     * @brief Set the camera parameters
     * @param params The new parameters
     */
    void setParams(const CameraParams& params);

    /**
     * @brief Get the camera position
     * @return The camera position in world space
     */
    Vec3 getPosition() const;

    /**
     * @brief Get the camera forward vector
     * @return The camera forward vector (normalized)
     */
    Vec3 getForward() const;

    /**
     * @brief Get the camera up vector
     * @return The camera up vector (normalized)
     */
    Vec3 getUp() const;

    /**
     * @brief Get the camera right vector
     * @return The camera right vector (normalized)
     */
    Vec3 getRight() const;

    /**
     * @brief Get the view matrix
     * @return The view matrix for rendering
     */
    Mat4 getViewMatrix() const;

    /**
     * @brief Get the projection matrix
     * @param aspectRatio The aspect ratio (width/height)
     * @return The projection matrix for rendering
     */
    Mat4 getProjectionMatrix(float aspectRatio) const;

private:
    // Camera parameters
    CameraParams m_params;
    
    // Camera state
    Vec3 m_position;     // Position in world space
    Vec3 m_target;       // Look-at target
    float m_yaw;         // Horizontal rotation in radians
    float m_pitch;       // Vertical rotation in radians
    
    // Camera vectors
    Vec3 m_forward;      // Forward vector
    Vec3 m_up;           // Up vector
    Vec3 m_right;        // Right vector
};

} // namespace Sylva 
#pragma once

#include "core/types.h"
#include <glm/glm.hpp>

namespace Sylva {

// Forward declarations
struct InputState;
class Shader;
class VoxelWorld;

/**
 * @brief Player system
 *
 * Handles player movement, collision, and rendering.
 * Player model uses larger voxel blocks to create visual contrast with the micro-voxel environment.
 */
class Player {
public:
    /**
     * @brief Constructor
     */
    Player();

    /**
     * @brief Constructor with parameters
     * @param params Player parameters to use
     */
    explicit Player(const PlayerParams& params);

    /**
     * @brief Destructor — releases the player mesh's GL handles.
     */
    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&) = delete;
    Player& operator=(Player&&) = delete;

    /**
     * @brief Update player movement against the world.
     * @param deltaTime  Time since last update.
     * @param input      Current input state.
     * @param cameraForward Horizontal forward vector from the camera (any Y is zeroed).
     * @param cameraRight   Horizontal right vector from the camera.
     * @param world      World to query for terrain height and collision.
     *
     * Player no longer depends on the Camera class — only on the two basis
     * vectors it needs, supplied by the caller.
     */
    void updateMovement(float deltaTime,
                        const InputState& input,
                        const Vec3& cameraForward,
                        const Vec3& cameraRight,
                        const VoxelWorld& world);

    /**
     * @brief Rotate to face a movement direction with smooth interpolation.
     */
    void rotateToMovementDirection(const Vec3& moveDirection, float deltaTime);

    /**
     * @brief Check collision with the voxel world
     * @param world VoxelWorld to check collision against
     * @return true if collision detected
     */
    bool checkCollision(const VoxelWorld& world) const;

    /**
     * @brief Render the player
     * @param shader The shader to use for rendering
     * @param viewMatrix The view matrix from the camera
     * @param projectionMatrix The projection matrix
     */
    void renderPlayer(Shader& shader, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    /**
     * @brief Set the player's move speed
     * @param speed The new move speed
     */
    void setMoveSpeed(float speed);

    /**
     * @brief Set the player's size
     * @param width The new width
     * @param depth The new depth
     */
    void setSize(float width, float depth);

    /**
     * @brief Set the player's color
     * @param color The new color (RGBA)
     */
    void setColor(const Vec4& color);

    /**
     * @brief Get the player's position
     * @return The player's position in world space
     */
    Vec3 getPosition() const;

    /**
     * @brief Get the player's rotation
     * @return The player's rotation in radians
     */
    float getRotation() const;

    /**
     * @brief Get the player's height
     * @return The player's height
     */
    float getHeight() const;

    /**
     * @brief Get the player's collision radius
     * @return The player's collision radius
     */
    float getCollisionRadius() const { return m_params.collisionRadius; }

    /**
     * @brief Set the player's jump height
     * @param height The maximum height of jumps
     */
    void setJumpHeight(float height);

    /**
     * @brief Set the player's jump force
     * @param force The initial vertical velocity applied when jumping
     */
    void setJumpForce(float force);

    /**
     * @brief Set the gravity strength
     * @param gravity The gravity acceleration value
     */
    void setGravity(float gravity);

    /**
     * @brief Check if the player is on the ground
     * @return True if the player is on the ground, false if in the air
     */
    bool isGrounded() const;

private:
    /**
     * @brief Process player input and calculate movement direction.
     * @param input Current input state.
     * @param cameraForward Horizontal camera-forward basis.
     * @param cameraRight   Horizontal camera-right basis.
     * @return Normalized movement direction vector.
     */
    Vec3 handlePlayerInput(const InputState& input, const Vec3& cameraForward, const Vec3& cameraRight);

    /**
     * @brief Update player position based on movement and physics
     * @param deltaTime Time since last frame
     * @param moveDirection Desired movement direction
     * @param world VoxelWorld for terrain height and collision
     * @return New position after movement
     */
    Vec3 updatePosition(float deltaTime, const Vec3& moveDirection, const VoxelWorld& world);

    /**
     * @brief Test if the player bbox would collide if it were at @p probe.
     *        Swaps m_position temporarily so checkCollision can run.
     */
    bool collidesAt(const Vec3& probe, const VoxelWorld& world);

    /**
     * @brief Try to move horizontally to (tx, tz). On block, attempt auto-step.
     *        Sets m_position and isGrounded / velocity if successful.
     * @return true if the player moved horizontally.
     */
    bool tryMoveTo(float tx, float tz, float fallbackY, const VoxelWorld& world);

    /**
     * @brief Update player animation state
     * @param deltaTime Time since last frame
     * @param moveDirection Current movement direction
     */
    void updateAnimation(float deltaTime, const Vec3& moveDirection);

    // Player parameters
    PlayerParams m_params;

    // Player state
    Vec3 m_position;  // Position in world space
    float m_rotation; // Rotation in radians
    Vec3 m_velocity;  // Current velocity

    /// Lazy-builds the GPU cube mesh on first render (needs a live GL context).
    void ensureMesh();

    // GPU resources for the placeholder cube model. Built on first render,
    // released in the destructor before the Engine tears down the window.
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    unsigned int m_indexCount = 0;
};

} // namespace Sylva
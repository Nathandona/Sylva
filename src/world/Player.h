#pragma once

#include "core/types.h"
#include <glm/glm.hpp>

namespace Sylva {

// Forward declarations
class World;
struct InputState;
class Shader;
class Camera;
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
    Player(const PlayerParams& params);
    
    /**
     * @brief Destructor
     */
    ~Player();

    /**
     * @brief Initialize player graphics
     * @return true if initialization successful
     */
    bool initializeGraphics();

    /**
     * @brief Update player movement with VoxelWorld
     * @param deltaTime Time since last update
     * @param input Input state
     * @param world VoxelWorld to check collisions against
     * @param camera Camera to use for relative movement
     */
    void updateMovement(float deltaTime, const InputState& input, const VoxelWorld& world, const Camera& camera);

    /**
     * @brief Rotate to face movement direction
     */
    void rotateToMovementDirection();

    /**
     * @brief Rotate to face a specific movement direction
     * @param moveDirection The movement direction to face
     * @param deltaTime Time since last frame for smooth rotation
     */
    void rotateToMovementDirection(const Vec3& moveDirection, float deltaTime);

    /**
     * @brief Check collision with the voxel world
     * @param world VoxelWorld to check collision against
     * @return true if collision detected
     */
    bool checkCollision(const VoxelWorld& world);

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
     * @brief Process player input and calculate movement direction
     * @param input Current input state
     * @param camera Camera for relative movement calculation
     * @return Normalized movement direction vector
     */
    Vec3 handlePlayerInput(const InputState& input, const Camera& camera);

    /**
     * @brief Update player position based on movement and physics
     * @param deltaTime Time since last frame
     * @param moveDirection Desired movement direction
     * @param world VoxelWorld for terrain height and collision
     * @return New position after movement
     */
    Vec3 updatePosition(float deltaTime, const Vec3& moveDirection, const VoxelWorld& world);

    /**
     * @brief Handle player collisions with the world
     * @param newPosition Desired new position
     * @param world VoxelWorld to check collisions against
     * @return True if collision detected, false otherwise
     */
    bool handleCollisions(const Vec3& newPosition, const VoxelWorld& world);

    /**
     * @brief Update player animation state
     * @param deltaTime Time since last frame
     * @param moveDirection Current movement direction
     */
    void updateAnimation(float deltaTime, const Vec3& moveDirection);

    // Player parameters
    PlayerParams m_params;
    
    // Player state
    Vec3 m_position;     // Position in world space
    float m_rotation;    // Rotation in radians
    Vec3 m_velocity;     // Current velocity
    
    // Graphics objects
    // unsigned int m_vao;  // Vertex Array Object - To be replaced by a Mesh component
    // unsigned int m_vbo;  // Vertex Buffer Object - To be replaced by a Mesh component
    // Shader* m_shader;    // Player shader - To be provided by a ShaderManager
    
    // Cleanup graphics objects
    // void cleanupGraphics(); // Handled by Mesh component and ShaderManager
};

} // namespace Sylva 
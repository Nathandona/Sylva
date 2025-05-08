#include "player.h"
#include "voxel_world.h"
#include "core/logger.h"
#include "core/config.h"
#include "renderer/shader.h"
#include "renderer/camera.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Sylva {

Player::Player() 
    : m_rotation(0.0f), 
      m_position(0.0f, 0.0f, 0.0f), 
      m_velocity(0.0f, 0.0f, 0.0f)
      // m_vao(0), // Managed by Mesh component
      // m_vbo(0), // Managed by Mesh component
      // m_shader(nullptr) // Provided by ShaderManager
{
    // Load player parameters from config if available
    m_params.moveSpeed = Config::getFloat("Player.move_speed", m_params.moveSpeed);
    m_params.rotationSpeed = Config::getFloat("Player.rotation_speed", m_params.rotationSpeed);
    m_params.collisionRadius = Config::getFloat("Player.collision_radius", m_params.collisionRadius);
    m_params.height = Config::getFloat("Player.height", m_params.height);
    m_params.width = Config::getFloat("Player.width", m_params.width);
    m_params.depth = Config::getFloat("Player.depth", m_params.depth);
    m_params.jumpHeight = Config::getFloat("Player.jump_height", m_params.jumpHeight);
    m_params.jumpForce = Config::getFloat("Player.jump_force", m_params.jumpForce);
    m_params.gravity = Config::getFloat("Player.gravity", m_params.gravity);
    
    Logger::logDebug("Player initialized at position: " + 
                    std::to_string(m_position.x) + ", " +
                    std::to_string(m_position.y) + ", " +
                    std::to_string(m_position.z));
}

Player::Player(const PlayerParams& params) 
    : m_params(params), 
      m_rotation(0.0f), 
      m_position(0.0f, 0.0f, 0.0f), 
      m_velocity(0.0f, 0.0f, 0.0f)
      // m_vao(0), // Managed by Mesh component
      // m_vbo(0), // Managed by Mesh component
      // m_shader(nullptr) // Provided by ShaderManager
{
    Logger::logDebug("Player initialized with custom parameters");
}

Player::~Player() {
    // cleanupGraphics(); // Handled by Mesh component and ShaderManager
}

bool Player::initializeGraphics() {
    Logger::logInfo("Initializing player graphics (conceptual setup)");
    
    // Shader creation is now handled by a ShaderManager
    // VAO/VBO and vertex data are now handled by a Mesh component

    // Example: Define player's visual properties if needed for a procedural mesh later
    // m_playerVisual.setColor(Vec4(0.2f, 0.5f, 1.0f, 1.0f));
    // m_playerVisual.setDimensions(m_params.width, m_params.height, m_params.depth);

    Logger::logInfo("Player graphics conceptually initialized. Mesh and Shader to be managed externally.");
    return true;
}

// void Player::cleanupGraphics() { // Handled by Mesh component and ShaderManager
//     if (m_vao != 0) {
//         glDeleteVertexArrays(1, &m_vao);
//         m_vao = 0;
//     }
//     
//     if (m_vbo != 0) {
//         glDeleteBuffers(1, &m_vbo);
//         m_vbo = 0;
//     }
//     
//     if (m_shader != nullptr) {
//         delete m_shader;
//         m_shader = nullptr;
//     }
// }

void Player::updateMovement(float deltaTime, const InputState& input, const VoxelWorld& world, const Camera& camera) {
    // Get camera forward and right vectors (for camera-relative movement)
    Vec3 forward = camera.getForward();
    Vec3 right = camera.getRight();
    
    // We only want horizontal movement, so zero out the Y component
    forward.y = 0.0f;
    right.y = 0.0f;
    
    // Normalize the vectors after zeroing out Y
    if (glm::length(forward) > 0.0f) {
        forward = glm::normalize(forward);
    }
    if (glm::length(right) > 0.0f) {
        right = glm::normalize(right);
    }
    
    // Calculate movement direction relative to camera orientation
    Vec3 moveDirection(0.0f, 0.0f, 0.0f);
    
    if (input.moveForward) {
        moveDirection += forward;
    }
    if (input.moveBackward) {
        moveDirection -= forward;
    }
    if (input.moveLeft) {
        moveDirection -= right;
    }
    if (input.moveRight) {
        moveDirection += right;
    }
    
    // Store original move direction for rotation
    Vec3 originalMoveDir = moveDirection;
    
    // Normalize movement vector if not zero
    if (glm::length(moveDirection) > 0.0f) {
        moveDirection = glm::normalize(moveDirection);
    }
    
    // Apply movement speed
    moveDirection *= m_params.moveSpeed * deltaTime;
    
    // Update horizontal position
    Vec3 newPosition = m_position;
    newPosition.x += moveDirection.x;
    newPosition.z += moveDirection.z;
    
    // Handle jumping and vertical movement
    float terrainHeight = world.getHeightAt(newPosition.x, newPosition.z);
    
    // Apply gravity and vertical velocity
    if (!m_params.isGrounded) {
        // Apply gravity to vertical velocity
        m_velocity.y -= m_params.gravity * deltaTime;
    }
    
    // Handle jump input
    if (input.jump && m_params.isGrounded) {
        // Apply jump force
        m_velocity.y = m_params.jumpForce;
        m_params.isGrounded = false;
        Logger::logDebug("Player jumped");
    }
    
    // Update vertical position with velocity
    newPosition.y += m_velocity.y * deltaTime;
    
    // Check if player is on or below ground
    if (newPosition.y <= terrainHeight) {
        newPosition.y = terrainHeight;
        m_velocity.y = 0.0f;
        m_params.isGrounded = true;
    } else {
        m_params.isGrounded = false;
    }
    
    // Check horizontal collision
    bool collision = false;
    if (moveDirection.x != 0.0f || moveDirection.z != 0.0f) {
        // Create a test position that only includes horizontal movement
        Vec3 testPosition = m_position;
        testPosition.x = newPosition.x;
        testPosition.z = newPosition.z;
        
        // Temporarily update position for collision check
        Vec3 oldPosition = m_position;
        m_position = testPosition;
        collision = checkCollision(world);
        m_position = oldPosition;  // Restore position
    }
    
    // If no horizontal collision, apply movement
    if (!collision) {
        m_position = newPosition;
    } else {
        // Just update the vertical position if horizontal collision occurred
        m_position.y = newPosition.y;
    }
    
    // Rotate to face movement direction if moving horizontally
    if (glm::length(originalMoveDir) > 0.0f) {
        rotateToMovementDirection(originalMoveDir, deltaTime);
    }
    
    Logger::logDebug("Player moved to: " + 
                    std::to_string(m_position.x) + ", " +
                    std::to_string(m_position.y) + ", " +
                    std::to_string(m_position.z) +
                    (m_params.isGrounded ? " (grounded)" : " (in air)"));
}

void Player::rotateToMovementDirection() {
    // This is the old method signature, kept for compatibility
    // The actual implementation is in the overloaded method below
    Logger::logDebug("Player rotated to movement direction");
}

void Player::rotateToMovementDirection(const Vec3& moveDirection, float deltaTime) {
    // Calculate the target yaw angle based on movement direction
    if (glm::length(moveDirection) > 0.0f) {
        // Calculate the angle in radians using atan2
        // Add PI to make the front face (-Z in local coords) point in the movement direction
        float targetRotation = atan2(moveDirection.x, moveDirection.z) + glm::pi<float>();
        
        // Smoothly interpolate to the target rotation
        float rotationDelta = m_params.rotationSpeed * deltaTime;
        
        // Find the shortest path to the target angle
        float angleDifference = targetRotation - m_rotation;
        
        // Normalize angle to -PI to PI
        while (angleDifference > glm::pi<float>()) {
            angleDifference -= 2.0f * glm::pi<float>();
        }
        while (angleDifference < -glm::pi<float>()) {
            angleDifference += 2.0f * glm::pi<float>();
        }
        
        // Apply smooth rotation with speed limit
        if (abs(angleDifference) < rotationDelta) {
            m_rotation = targetRotation; // Close enough, snap to target
        } else {
            // Move towards the target at limited speed
            m_rotation += (angleDifference > 0.0f) ? rotationDelta : -rotationDelta;
        }
        
        // Keep rotation in range [0, 2π]
        while (m_rotation < 0.0f) {
            m_rotation += 2.0f * glm::pi<float>();
        }
        while (m_rotation >= 2.0f * glm::pi<float>()) {
            m_rotation -= 2.0f * glm::pi<float>();
        }
        
        Logger::logDebug("Player rotated to " + std::to_string(m_rotation) + " radians");
    }
}

bool Player::checkCollision(const VoxelWorld& world) {
    // Delegate to voxel world's collision checking
    return world.checkCollision(*this);
}

void Player::renderPlayer(Shader& shader, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    shader.use();
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_position);
    model = glm::rotate(model, m_rotation, glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around Y axis
    
    shader.setMat4("model", model);
    shader.setMat4("view", viewMatrix);
    shader.setMat4("projection", projectionMatrix);
    // shader.setVec4("color", m_params.color); // Assuming color is part of PlayerParams or a visual component

    // TODO: Draw player mesh here using the provided shader and a Mesh component.
    // Example: if (m_mesh) m_mesh->draw(shader);
    
    // Removed direct OpenGL draw calls:
    // glBindVertexArray(m_vao);
    // glDrawArrays(GL_TRIANGLES, 0, 36); // 36 vertices for a cube (12 triangles * 3 vertices)
    // glBindVertexArray(0);
}

void Player::setMoveSpeed(float speed) {
    m_params.moveSpeed = speed;
}

void Player::setSize(float width, float depth) {
    m_params.width = width;
    m_params.depth = depth;
}

void Player::setColor(const Vec4& color) {
    m_params.color = color;
}

Vec3 Player::getPosition() const {
    return m_position;
}

float Player::getRotation() const {
    return m_rotation;
}

float Player::getHeight() const {
    return m_params.height;
}

void Player::setJumpHeight(float height) {
    m_params.jumpHeight = height;
    
    // Recalculate jump force based on jumpHeight and gravity
    // Using physics formula: v = sqrt(2 * g * h)
    m_params.jumpForce = sqrt(2.0f * m_params.gravity * m_params.jumpHeight);
}

void Player::setJumpForce(float force) {
    m_params.jumpForce = force;
    
    // Update jump height to match the new force
    // Using physics formula: h = v^2 / (2 * g)
    m_params.jumpHeight = (m_params.jumpForce * m_params.jumpForce) / (2.0f * m_params.gravity);
}

void Player::setGravity(float gravity) {
    m_params.gravity = gravity;
    
    // Recalculate jump force to maintain the same jump height
    m_params.jumpForce = sqrt(2.0f * m_params.gravity * m_params.jumpHeight);
}

bool Player::isGrounded() const {
    return m_params.isGrounded;
}

} // namespace Sylva 
#include "player.h"
#include "voxel_world.h"
#include "core/logger.h"
#include "core/config.h"
#include "renderer/shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Sylva {

Player::Player() : m_position(0.0f, 0.0f, 0.0f), m_rotation(0.0f), m_velocity(0.0f, 0.0f, 0.0f) {
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

    Logger::logDebug("Player initialized at position: " + std::to_string(m_position.x) + ", " +
                     std::to_string(m_position.y) + ", " + std::to_string(m_position.z));
}

Player::Player(const PlayerParams& params)
    : m_params(params), m_position(0.0f, 0.0f, 0.0f), m_rotation(0.0f), m_velocity(0.0f, 0.0f, 0.0f) {
    Logger::logDebug("Player initialized with custom parameters");
}

Vec3 Player::handlePlayerInput(const InputState& input, const Vec3& cameraForward, const Vec3& cameraRight) {
    // Project to horizontal plane and renormalize. Length() == 0 means the
    // caller passed a degenerate vector (e.g. looking straight up) — fall
    // back to zero contribution rather than dividing by zero.
    auto flatten = [](Vec3 v) {
        v.y = 0.0f;
        const float len = glm::length(v);
        return (len > 0.0f) ? v / len : Vec3(0.0f);
    };
    const Vec3 forward = flatten(cameraForward);
    const Vec3 right = flatten(cameraRight);

    Vec3 moveDirection(0.0f);
    if (input.moveForward)
        moveDirection += forward;
    if (input.moveBackward)
        moveDirection -= forward;
    if (input.moveLeft)
        moveDirection -= right;
    if (input.moveRight)
        moveDirection += right;
    if (glm::length(moveDirection) > 0.0f) {
        moveDirection = glm::normalize(moveDirection);
    }
    return moveDirection;
}

Vec3 Player::updatePosition(float deltaTime, const Vec3& moveDirection, const VoxelWorld& world) {
    // Apply movement speed to horizontal movement
    Vec3 const scaledMove = moveDirection * m_params.moveSpeed * deltaTime;

    // Calculate new position
    Vec3 newPosition = m_position;
    newPosition.x += scaledMove.x;
    newPosition.z += scaledMove.z;

    // Handle vertical movement
    float const terrainHeight = world.getHeightAt(newPosition.x, newPosition.z);

    // Apply gravity when not grounded
    if (!m_params.isGrounded) {
        m_velocity.y -= m_params.gravity * deltaTime;
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

    return newPosition;
}

bool Player::handleCollisions(const Vec3& newPosition, const VoxelWorld& world) {
    // Only check horizontal collision if there's horizontal movement
    if (newPosition.x != m_position.x || newPosition.z != m_position.z) {
        // Create a test position that only includes horizontal movement
        Vec3 testPosition = m_position;
        testPosition.x = newPosition.x;
        testPosition.z = newPosition.z;

        // Temporarily update position for collision check
        Vec3 const oldPosition = m_position;
        m_position = testPosition;
        bool const collision = checkCollision(world);
        m_position = oldPosition; // Restore position

        return collision;
    }

    return false;
}

void Player::updateAnimation(float deltaTime, const Vec3& moveDirection) {
    if (glm::length(moveDirection) > 0.0f) {
        rotateToMovementDirection(moveDirection, deltaTime);
    }
}

void Player::updateMovement(float deltaTime,
                            const InputState& input,
                            const Vec3& cameraForward,
                            const Vec3& cameraRight,
                            const VoxelWorld& world) {
    Vec3 const moveDirection = handlePlayerInput(input, cameraForward, cameraRight);

    // Handle jumping
    if (input.jump && m_params.isGrounded) {
        m_velocity.y = m_params.jumpForce;
        m_params.isGrounded = false;
        Logger::logDebug("Player jumped");
    }

    // Calculate new position based on movement and physics
    Vec3 const newPosition = updatePosition(deltaTime, moveDirection, world);

    // Check for collisions
    bool const collision = handleCollisions(newPosition, world);

    // Apply movement if no collision, or just vertical movement if collision
    if (!collision) {
        m_position = newPosition;
    } else {
        m_position.y = newPosition.y;
    }

    // Update animation state
    updateAnimation(deltaTime, moveDirection);
}

void Player::rotateToMovementDirection(const Vec3& moveDirection, float deltaTime) {
    // Calculate the target yaw angle based on movement direction
    if (glm::length(moveDirection) > 0.0f) {
        // Calculate the angle in radians using atan2
        // Add PI to make the front face (-Z in local coords) point in the movement direction
        float const targetRotation = atan2(moveDirection.x, moveDirection.z) + glm::pi<float>();

        // Smoothly interpolate to the target rotation
        float const rotationDelta = m_params.rotationSpeed * deltaTime;

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
    }
}

bool Player::checkCollision(const VoxelWorld& world) const {
    // Delegate to voxel world's collision checking
    return world.checkCollision(*this);
}

void Player::renderPlayer(Shader& shader, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    shader.use();

    auto model = glm::mat4(1.0f);
    model = glm::translate(model, m_position);
    model = glm::rotate(model, m_rotation, glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around Y axis

    shader.setMat4("model", model);
    shader.setMat4("view", viewMatrix);
    shader.setMat4("projection", projectionMatrix);
    // TODO: draw via a Mesh component once introduced.
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
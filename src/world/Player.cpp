#include "player.h"
#include "voxel_world.h"
#include "character_model.h"
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
    m_params.autoStepHeight = Config::getFloat("Player.auto_step_height", m_params.autoStepHeight);

    Logger::logDebug("Player initialized at position: " + std::to_string(m_position.x) + ", " +
                     std::to_string(m_position.y) + ", " + std::to_string(m_position.z));
}

Player::Player(const PlayerParams& params)
    : m_params(params), m_position(0.0f, 0.0f, 0.0f), m_rotation(0.0f), m_velocity(0.0f, 0.0f, 0.0f) {
    Logger::logDebug("Player initialized with custom parameters");
}

Player::~Player() {
    // m_model destroyed via unique_ptr; Engine::shutdown order ensures the
    // GL context still exists when this dtor runs.
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
    const Vec3 scaledMove = moveDirection * m_params.moveSpeed * deltaTime;

    Vec3 newPosition = m_position;
    newPosition.x += scaledMove.x;
    newPosition.z += scaledMove.z;

    const float terrainHeight = world.getHeightAt(newPosition.x, newPosition.z);
    const bool wasGrounded = m_params.isGrounded;

    if (!m_params.isGrounded) {
        m_velocity.y -= m_params.gravity * deltaTime;
    }
    newPosition.y += m_velocity.y * deltaTime;

    if (newPosition.y <= terrainHeight) {
        newPosition.y = terrainHeight;
        m_velocity.y = 0.0f;
        m_params.isGrounded = true;
    } else {
        // Just transitioned from grounded to airborne via a ledge step-off.
        // Without a starting downward velocity, gravity has to ramp from zero
        // and the player visibly hangs in the air for ~0.18s before falling
        // one voxel. Seed the fall so it feels instant.
        if (wasGrounded && m_velocity.y >= 0.0f) {
            m_velocity.y = -2.0f; // ~1-voxel drop in ~130ms before gravity adds more
        }
        m_params.isGrounded = false;
    }

    return newPosition;
}

bool Player::collidesAt(const Vec3& probe, const VoxelWorld& world) {
    const Vec3 saved = m_position;
    m_position = probe;
    const bool c = checkCollision(world);
    m_position = saved;
    return c;
}

bool Player::tryMoveTo(float tx, float tz, float fallbackY, const VoxelWorld& world) {
    // Pick a landing Y. While grounded, snap to terrain at the new XZ so an
    // axis-slide along a slope still tracks the ground. While airborne, honor
    // the caller's Y (gravity-resolved).
    float ty = fallbackY;
    if (m_params.isGrounded) {
        ty = world.getHeightAt(tx, tz);
    }

    const Vec3 target(tx, ty, tz);
    if (!collidesAt(target, world)) {
        m_position = target;
        return true;
    }

    // Blocked at the desired landing. Try auto-step.
    if (m_params.isGrounded && m_params.autoStepHeight > 0.0f) {
        const Vec3 stepped(tx, m_position.y + m_params.autoStepHeight, tz);
        if (!collidesAt(stepped, world)) {
            // Body fits when raised by step height. Land on actual terrain.
            m_position.x = tx;
            m_position.z = tz;
            m_position.y = world.getHeightAt(tx, tz);
            m_velocity.y = 0.0f;
            m_params.isGrounded = true;
            return true;
        }
    }
    return false;
}

void Player::updateAnimation(float deltaTime, const Vec3& moveDirection) {
    if (glm::length(moveDirection) > 0.0f) {
        rotateToMovementDirection(moveDirection, deltaTime);
    }
    if (m_model) {
        const auto state = (glm::length(moveDirection) > 0.0f) ? CharacterAnimState::Walk : CharacterAnimState::Idle;
        m_model->update(deltaTime, state);
    }
}

void Player::updateMovement(float deltaTime,
                            const InputState& input,
                            const Vec3& cameraForward,
                            const Vec3& cameraRight,
                            const VoxelWorld& world) {
    const Vec3 moveDirection = handlePlayerInput(input, cameraForward, cameraRight);

    if (input.jump && m_params.isGrounded) {
        m_velocity.y = m_params.jumpForce;
        m_params.isGrounded = false;
    }

    const Vec3 desired = updatePosition(deltaTime, moveDirection, world);

    // 1. Try the full combined XZ move.
    bool moved = tryMoveTo(desired.x, desired.z, desired.y, world);

    // 2. If blocked, slide along each axis independently. Diagonal-into-corner
    //    previously stopped both axes; now if one axis is clear the player
    //    still slides along it.
    if (!moved) {
        const bool xMoved = tryMoveTo(desired.x, m_position.z, desired.y, world);
        const bool zMoved = tryMoveTo(m_position.x, desired.z, desired.y, world);
        moved = xMoved || zMoved;
    }

    // 3. No horizontal progress at all — apply vertical motion (gravity/jump)
    //    standalone. tryMoveTo already sets Y when it succeeds.
    if (!moved) {
        m_position.y = desired.y;
    }

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
    // Lazy-init the character model on first render (needs a live GL context;
    // can't be done in the constructor because Engine creates Player before
    // it owns a window).
    if (!m_model) {
        m_model = std::make_unique<CharacterModel>();
    }

    // Root transform: translate to player position, then yaw. The character
    // model lives in world units (~1.8m tall) so no scale is applied.
    glm::mat4 root = glm::translate(glm::mat4(1.0f), m_position);
    root = glm::rotate(root, m_rotation, glm::vec3(0.0f, 1.0f, 0.0f));

    m_model->render(shader, root, viewMatrix, projectionMatrix);
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
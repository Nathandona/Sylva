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
      m_velocity(0.0f, 0.0f, 0.0f),
      m_vao(0),
      m_vbo(0),
      m_shader(nullptr) {
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
      m_velocity(0.0f, 0.0f, 0.0f),
      m_vao(0),
      m_vbo(0),
      m_shader(nullptr) {
    Logger::logDebug("Player initialized with custom parameters");
}

Player::~Player() {
    cleanupGraphics();
}

bool Player::initializeGraphics() {
    Logger::logInfo("Initializing player graphics");
    
    // Create shader - use the new colored shader
    try {
        m_shader = new Shader("assets/shaders/colored.vert", "assets/shaders/colored.frag");
    } catch (const std::exception& e) {
        Logger::logError("Failed to load player shader: " + std::string(e.what()));
        return false;
    }
    
    // Generate and bind VAO/VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    
    // Create a simple rectangular prism for the player
    float halfWidth = m_params.width / 2.0f;
    float halfDepth = m_params.depth / 2.0f;
    float height = m_params.height;
    
    // Define colors for each face
    // RGB values - the alpha will be handled in the shader
    const float frontColor[3] = {0.0f, 1.0f, 0.0f};   // Green (front)
    const float backColor[3] = {1.0f, 0.0f, 0.0f};    // Red (back)
    const float leftColor[3] = {0.0f, 0.0f, 1.0f};    // Blue (left)
    const float rightColor[3] = {1.0f, 1.0f, 0.0f};   // Yellow (right)
    const float bottomColor[3] = {0.0f, 1.0f, 1.0f};  // Cyan (bottom)
    const float topColor[3] = {1.0f, 0.0f, 1.0f};     // Magenta (top)
    
    // Vertices for a rectangular prism (12 triangles, 36 vertices with positions, colors, and tex coords)
    float vertices[] = {
        // Positions                // Colors              // Texture Coords
        // Front face (Green)
        -halfWidth, 0.0f, -halfDepth,  frontColor[0], frontColor[1], frontColor[2],  0.0f, 0.0f,
         halfWidth, 0.0f, -halfDepth,  frontColor[0], frontColor[1], frontColor[2],  1.0f, 0.0f,
         halfWidth, height, -halfDepth,  frontColor[0], frontColor[1], frontColor[2],  1.0f, 1.0f,
        -halfWidth, 0.0f, -halfDepth,  frontColor[0], frontColor[1], frontColor[2],  0.0f, 0.0f,
         halfWidth, height, -halfDepth,  frontColor[0], frontColor[1], frontColor[2],  1.0f, 1.0f,
        -halfWidth, height, -halfDepth,  frontColor[0], frontColor[1], frontColor[2],  0.0f, 1.0f,

        // Back face (Red)
        -halfWidth, 0.0f, halfDepth,   backColor[0], backColor[1], backColor[2],  0.0f, 0.0f,
         halfWidth, 0.0f, halfDepth,   backColor[0], backColor[1], backColor[2],  1.0f, 0.0f,
         halfWidth, height, halfDepth,   backColor[0], backColor[1], backColor[2],  1.0f, 1.0f,
        -halfWidth, 0.0f, halfDepth,   backColor[0], backColor[1], backColor[2],  0.0f, 0.0f,
         halfWidth, height, halfDepth,   backColor[0], backColor[1], backColor[2],  1.0f, 1.0f,
        -halfWidth, height, halfDepth,   backColor[0], backColor[1], backColor[2],  0.0f, 1.0f,

        // Left face (Blue)
        -halfWidth, 0.0f, -halfDepth,  leftColor[0], leftColor[1], leftColor[2],  0.0f, 0.0f,
        -halfWidth, 0.0f, halfDepth,   leftColor[0], leftColor[1], leftColor[2],  1.0f, 0.0f,
        -halfWidth, height, halfDepth,   leftColor[0], leftColor[1], leftColor[2],  1.0f, 1.0f,
        -halfWidth, 0.0f, -halfDepth,  leftColor[0], leftColor[1], leftColor[2],  0.0f, 0.0f,
        -halfWidth, height, halfDepth,   leftColor[0], leftColor[1], leftColor[2],  1.0f, 1.0f,
        -halfWidth, height, -halfDepth,  leftColor[0], leftColor[1], leftColor[2],  0.0f, 1.0f,

        // Right face (Yellow)
        halfWidth, 0.0f, -halfDepth,   rightColor[0], rightColor[1], rightColor[2],  0.0f, 0.0f,
        halfWidth, 0.0f, halfDepth,    rightColor[0], rightColor[1], rightColor[2],  1.0f, 0.0f,
        halfWidth, height, halfDepth,    rightColor[0], rightColor[1], rightColor[2],  1.0f, 1.0f,
        halfWidth, 0.0f, -halfDepth,   rightColor[0], rightColor[1], rightColor[2],  0.0f, 0.0f,
        halfWidth, height, halfDepth,    rightColor[0], rightColor[1], rightColor[2],  1.0f, 1.0f,
        halfWidth, height, -halfDepth,   rightColor[0], rightColor[1], rightColor[2],  0.0f, 1.0f,

        // Bottom face (Cyan)
        -halfWidth, 0.0f, -halfDepth,  bottomColor[0], bottomColor[1], bottomColor[2],  0.0f, 0.0f,
         halfWidth, 0.0f, -halfDepth,  bottomColor[0], bottomColor[1], bottomColor[2],  1.0f, 0.0f,
         halfWidth, 0.0f, halfDepth,   bottomColor[0], bottomColor[1], bottomColor[2],  1.0f, 1.0f,
        -halfWidth, 0.0f, -halfDepth,  bottomColor[0], bottomColor[1], bottomColor[2],  0.0f, 0.0f,
         halfWidth, 0.0f, halfDepth,   bottomColor[0], bottomColor[1], bottomColor[2],  1.0f, 1.0f,
        -halfWidth, 0.0f, halfDepth,   bottomColor[0], bottomColor[1], bottomColor[2],  0.0f, 1.0f,

        // Top face (Magenta)
        -halfWidth, height, -halfDepth,  topColor[0], topColor[1], topColor[2],  0.0f, 0.0f,
         halfWidth, height, -halfDepth,  topColor[0], topColor[1], topColor[2],  1.0f, 0.0f,
         halfWidth, height, halfDepth,   topColor[0], topColor[1], topColor[2],  1.0f, 1.0f,
        -halfWidth, height, -halfDepth,  topColor[0], topColor[1], topColor[2],  0.0f, 0.0f,
         halfWidth, height, halfDepth,   topColor[0], topColor[1], topColor[2],  1.0f, 1.0f,
        -halfWidth, height, halfDepth,   topColor[0], topColor[1], topColor[2],  0.0f, 1.0f
    };
    
    // Bind and upload data
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    Logger::logInfo("Player graphics initialized with colored faces");
    return true;
}

void Player::cleanupGraphics() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    
    if (m_shader != nullptr) {
        delete m_shader;
        m_shader = nullptr;
    }
}

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

void Player::renderPlayer(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    if (m_shader == nullptr || m_vao == 0) {
        Logger::logWarning("Cannot render player, graphics not initialized");
        return;
    }
    
    // Use player shader
    m_shader->use();
    
    // Set matrices
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(m_position.x, m_position.y, m_position.z));
    modelMatrix = glm::rotate(modelMatrix, m_rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    
    m_shader->setMat4("model", modelMatrix);
    m_shader->setMat4("view", viewMatrix);
    m_shader->setMat4("projection", projectionMatrix);
    
    // Store current face culling state
    GLboolean cullFace;
    glGetBooleanv(GL_CULL_FACE, &cullFace);
    
    // Disable face culling for player rendering
    glDisable(GL_CULL_FACE);
    
    // Draw player
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36); // 12 triangles, 36 vertices
    glBindVertexArray(0);
    
    // Restore previous face culling state
    if (cullFace) {
        glEnable(GL_CULL_FACE);
    }
    
    Logger::logDebug("Player rendered at position: " + 
                    std::to_string(m_position.x) + ", " +
                    std::to_string(m_position.y) + ", " +
                    std::to_string(m_position.z));
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
#include "Player.h"
#include "World.h"
#include "../renderer/Renderer.h"
#include "../renderer/Mesh.h"
#include "../renderer/Shader.h"
#include "../renderer/ResourceManager.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <algorithm>

namespace Sylva {

Player::Player()
    : m_Position(0.0f, 10.0f, 0.0f)
    , m_Velocity(0.0f)
    , m_Yaw(0.0f) // Initialize Yaw (facing positive Z initially)
    , m_WalkSpeed(3.0f)
    , m_RunSpeed(6.0f)
    , m_TurnSpeed(180.0f) // 180 degrees per second
    , m_JumpForce(8.0f)
    , m_Gravity(20.0f)
    , m_IsGrounded(false)
    , m_GroundCheckDistance(0.2f)
    , m_Renderer(nullptr)
    , m_Mesh(nullptr)
    , m_Shader(nullptr)
    , m_Color(1.0f, 1.0f, 1.0f)
{
}

Player::~Player() {
    // Resources handled by smart pointers
}

bool Player::Initialize(Renderer* renderer) {
    if (!renderer) {
        std::cerr << "Cannot initialize Player without a valid Renderer!" << std::endl;
        return false;
    }
    
    m_Renderer = renderer;
    
    // Get resource manager from renderer
    ResourceManager* resourceManager = m_Renderer->GetResourceManager();
    if (!resourceManager) {
        std::cerr << "Error: Renderer does not have a ResourceManager!" << std::endl;
        return false;
    }
    
    // Load shader
    m_Shader = resourceManager->GetShader("assets/shaders/basic.vert", 
                                        "assets/shaders/basic.frag");
    if (!m_Shader) {
        std::cerr << "Failed to load player shader!" << std::endl;
        return false;
    }
    
    // Create a simple rectangular mesh for the player (box shape)
    const float width = 1.0f;
    const float height = 2.0f;
    const float depth = 1.0f;
    
    // Define the 8 corners of a cube
    glm::vec3 p0(-width/2, 0.0f, -depth/2);
    glm::vec3 p1( width/2, 0.0f, -depth/2);
    glm::vec3 p2( width/2, 0.0f,  depth/2);
    glm::vec3 p3(-width/2, 0.0f,  depth/2);
    glm::vec3 p4(-width/2, height, -depth/2);
    glm::vec3 p5( width/2, height, -depth/2);
    glm::vec3 p6( width/2, height,  depth/2);
    glm::vec3 p7(-width/2, height,  depth/2);
    
    std::vector<float> vertices = {
        // Positions (XYZ)           Colors (RGB)
        // Bottom face
        p0.x, p0.y, p0.z,           m_Color.r, m_Color.g, m_Color.b,
        p1.x, p1.y, p1.z,           m_Color.r, m_Color.g, m_Color.b,
        p2.x, p2.y, p2.z,           m_Color.r, m_Color.g, m_Color.b,
        p3.x, p3.y, p3.z,           m_Color.r, m_Color.g, m_Color.b,
        
        // Top face
        p4.x, p4.y, p4.z,           m_Color.r, m_Color.g, m_Color.b,
        p5.x, p5.y, p5.z,           m_Color.r, m_Color.g, m_Color.b,
        p6.x, p6.y, p6.z,           m_Color.r, m_Color.g, m_Color.b,
        p7.x, p7.y, p7.z,           m_Color.r, m_Color.g, m_Color.b,
        
        // Front face
        p3.x, p3.y, p3.z,           m_Color.r, m_Color.g, m_Color.b,
        p2.x, p2.y, p2.z,           m_Color.r, m_Color.g, m_Color.b,
        p6.x, p6.y, p6.z,           m_Color.r, m_Color.g, m_Color.b,
        p7.x, p7.y, p7.z,           m_Color.r, m_Color.g, m_Color.b,
        
        // Back face
        p0.x, p0.y, p0.z,           m_Color.r, m_Color.g, m_Color.b,
        p4.x, p4.y, p4.z,           m_Color.r, m_Color.g, m_Color.b,
        p5.x, p5.y, p5.z,           m_Color.r, m_Color.g, m_Color.b,
        p1.x, p1.y, p1.z,           m_Color.r, m_Color.g, m_Color.b,
        
        // Left face
        p0.x, p0.y, p0.z,           m_Color.r, m_Color.g, m_Color.b,
        p3.x, p3.y, p3.z,           m_Color.r, m_Color.g, m_Color.b,
        p7.x, p7.y, p7.z,           m_Color.r, m_Color.g, m_Color.b,
        p4.x, p4.y, p4.z,           m_Color.r, m_Color.g, m_Color.b,
        
        // Right face
        p1.x, p1.y, p1.z,           m_Color.r, m_Color.g, m_Color.b,
        p5.x, p5.y, p5.z,           m_Color.r, m_Color.g, m_Color.b,
        p6.x, p6.y, p6.z,           m_Color.r, m_Color.g, m_Color.b,
        p2.x, p2.y, p2.z,           m_Color.r, m_Color.g, m_Color.b
    };
    
    std::vector<unsigned int> indices = {
        // Bottom face
        0, 1, 2,
        2, 3, 0,
        
        // Top face
        4, 7, 6,
        6, 5, 4,
        
        // Front face
        8, 9, 10,
        10, 11, 8,
        
        // Back face
        12, 13, 14,
        14, 15, 12,
        
        // Left face
        16, 17, 18,
        18, 19, 16,
        
        // Right face
        20, 21, 22,
        22, 23, 20
    };
    
    // Create the mesh
    m_Mesh = std::make_unique<Mesh>();
    m_Mesh->SetVertexData(vertices, indices, 6, 0, -1, -1, 3);
    
    return true;
}

// New Update method with InputManager
void Player::Update(float deltaTime, const Platform* platform, World* world, InputManager* inputManager) {
    // Validate inputs
    if (!platform || !world || !inputManager) {
        return;
    }
    
    // Handle input using the input manager
    HandleInputWithManager(deltaTime, inputManager);
    
    // Apply gravity if not grounded
    if (!m_IsGrounded) {
        ApplyGravity(deltaTime);
    } else {
        // Prevent sliding down slopes when grounded but not moving vertically
        if (m_Velocity.y < 0.0f) {
            m_Velocity.y = 0.0f;
        }
    }
    
    // Update position based on velocity
    m_Position += m_Velocity * deltaTime;
    
    // Reset horizontal velocity each frame (movement is applied instantly via input)
    // Keep vertical velocity for gravity/jumping
    m_Velocity.x = 0.0f;
    m_Velocity.z = 0.0f;
    
    // Check for ground collision
    CheckGroundCollision(world);
    
    // Ensure player stays within world bounds
    m_Position.x = std::max(World::WORLD_MIN_X + 1.0f, std::min(m_Position.x, World::WORLD_MAX_X - 1.0f));
    m_Position.z = std::max(World::WORLD_MIN_Z + 1.0f, std::min(m_Position.z, World::WORLD_MAX_Z - 1.0f));
}

void Player::HandleInput(float deltaTime, const Platform* platform) {
    if (!platform) return;
    
    glm::vec3 moveDirection(0.0f);
    float currentSpeed = m_WalkSpeed;
    
    // --- Turning ---
    if (platform->IsKeyPressed(GLFW_KEY_Q)) { // Turn left
        m_Yaw -= m_TurnSpeed * deltaTime; 
    }
    if (platform->IsKeyPressed(GLFW_KEY_E)) { // Turn right
        m_Yaw += m_TurnSpeed * deltaTime; 
    }

    // Normalize Yaw to keep it within [0, 360) degrees
    m_Yaw = fmod(m_Yaw, 360.0f);
    if (m_Yaw < 0.0f) m_Yaw += 360.0f;

    // Convert yaw to radians for direction calculations
    float yawRad = glm::radians(m_Yaw);
    
    // Calculate forward and right vectors based on player's orientation
    glm::vec3 forward = glm::normalize(glm::vec3(sin(yawRad), 0.0f, cos(yawRad)));
    glm::vec3 right = glm::normalize(glm::vec3(sin(yawRad + glm::radians(90.0f)), 0.0f, cos(yawRad + glm::radians(90.0f))));
    
    // Movement controls
    if (platform->IsKeyPressed(GLFW_KEY_W)) {
        moveDirection += forward; // Move forward
    }
    if (platform->IsKeyPressed(GLFW_KEY_S)) {
        moveDirection -= forward; // Move backward
    }
    if (platform->IsKeyPressed(GLFW_KEY_A)) {
        moveDirection -= right; // Strafe left
    }
    if (platform->IsKeyPressed(GLFW_KEY_D)) {
        moveDirection += right; // Strafe right
    }
    
    // --- Running ---
    if (platform->IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
        currentSpeed = m_RunSpeed;
    }
    
    // Apply movement velocity
    if (glm::length(moveDirection) > 0.01f) {
        moveDirection = glm::normalize(moveDirection);
        m_Velocity.x = moveDirection.x * currentSpeed;
        m_Velocity.z = moveDirection.z * currentSpeed;
    }
    
    // --- Jumping ---
    if (m_IsGrounded && platform->IsKeyPressed(GLFW_KEY_SPACE)) {
        m_Velocity.y = m_JumpForce;
        m_IsGrounded = false;
    }
}

void Player::HandleInputWithManager(float deltaTime, InputManager* inputManager) {
    if (!inputManager) return;
    
    glm::vec3 moveDirection(0.0f);
    float currentSpeed = m_WalkSpeed;
    
    // --- Turning ---
    if (inputManager->IsActionPressed("TurnLeft")) { // Turn left
        m_Yaw -= m_TurnSpeed * deltaTime; 
    }
    if (inputManager->IsActionPressed("TurnRight")) { // Turn right
        m_Yaw += m_TurnSpeed * deltaTime; 
    }

    // Normalize Yaw to keep it within [0, 360) degrees
    m_Yaw = fmod(m_Yaw, 360.0f);
    if (m_Yaw < 0.0f) m_Yaw += 360.0f;

    // Convert yaw to radians for direction calculations
    float yawRad = glm::radians(m_Yaw);
    
    // Calculate forward and right vectors based on player's orientation
    glm::vec3 forward = glm::normalize(glm::vec3(sin(yawRad), 0.0f, cos(yawRad)));
    glm::vec3 right = glm::normalize(glm::vec3(sin(yawRad + glm::radians(90.0f)), 0.0f, cos(yawRad + glm::radians(90.0f))));
    
    // Get movement direction using axis values
    float forwardAxis = inputManager->GetAxisValue("MoveForward", "MoveBackward");
    float rightAxis = inputManager->GetAxisValue("MoveRight", "MoveLeft");
    
    // Apply movement based on axes
    if (forwardAxis != 0.0f) {
        moveDirection += forward * forwardAxis;
    }
    if (rightAxis != 0.0f) {
        moveDirection += right * rightAxis;
    }
    
    // --- Running ---
    if (inputManager->IsActionPressed("Run")) {
        currentSpeed = m_RunSpeed;
    }
    
    // Apply movement velocity
    if (glm::length(moveDirection) > 0.01f) {
        moveDirection = glm::normalize(moveDirection);
        m_Velocity.x = moveDirection.x * currentSpeed;
        m_Velocity.z = moveDirection.z * currentSpeed;
    }
    
    // --- Jumping ---
    if (m_IsGrounded && inputManager->IsActionPressed("Jump")) {
        m_Velocity.y = m_JumpForce;
        m_IsGrounded = false;
    }
}

void Player::ApplyGravity(float deltaTime) {
    m_Velocity.y -= m_Gravity * deltaTime;
    // Optional: Clamp to terminal velocity
    m_Velocity.y = std::max(m_Velocity.y, -30.0f);
}

void Player::CheckGroundCollision(World* world) {
    if (!world) {
        m_IsGrounded = false;
        return;
    }
    
    float terrainHeight = world->GetTerrainHeightAt(m_Position.x, m_Position.z);
    
    // Adjust for player height - feet are at the bottom of the model
    float playerFeetY = m_Position.y;
    
    if (playerFeetY <= terrainHeight + m_GroundCheckDistance && m_Velocity.y <= 0.0f) {
        // Snap position to ground and reset vertical velocity
        m_Position.y = terrainHeight;
        m_Velocity.y = 0.0f;
        m_IsGrounded = true;
    } else {
        m_IsGrounded = false;
    }
}

float Player::GetYaw() const {
    return m_Yaw;
}

void Player::SetPosition(const glm::vec3& position) {
    m_Position = position;
    m_Velocity = glm::vec3(0.0f); // Reset velocity on position change
    m_IsGrounded = false; // Force recheck of ground status
}

void Player::Render(Camera* camera) {
    if (!m_Mesh || !m_Shader || !camera) {
        return;
    }
    
    // Use the shader
    m_Shader->Use();
    
    // Set view and projection matrices from camera
    m_Shader->SetMat4("view", camera->GetViewMatrix());
    m_Shader->SetMat4("projection", camera->GetProjectionMatrix());
    
    // Create model matrix
    glm::mat4 model = glm::mat4(1.0f);
    
    // Apply translation
    model = glm::translate(model, m_Position);
    
    // Apply rotation around Y axis (now using m_Yaw in degrees)
    model = glm::rotate(model, glm::radians(m_Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Set the model matrix in the shader
    m_Shader->SetMat4("model", model);
    
    // Draw the mesh
    m_Mesh->Draw();
}

} // namespace Sylva 
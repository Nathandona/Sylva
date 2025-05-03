#include "Player.h"
#include "World.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace Sylva {

Player::Player()
    : m_Position(0.0f, 1.0f, 0.0f)
    , m_Velocity(0.0f)
    , m_RotationY(0.0f)
    , m_Speed(10.0f)
    , m_JumpForce(8.0f)
    , m_Gravity(20.0f)
    , m_IsGrounded(false)
    , m_Renderer(nullptr)
    , m_Mesh(nullptr)
    , m_Shader(nullptr)
    , m_Color(1.0f, 1.0f, 1.0f) // Bright white color for maximum visibility
{
}

Player::~Player() {
    // We don't own the renderer, so don't delete it
    // m_Mesh is now handled by the unique_ptr
    // m_Shader is now handled by the shared_ptr
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
    // The rectangle is 3x8x3 units (width x height x depth) - much larger for visibility
    std::vector<float> vertices = {
        // Position           Normal             Color
        // Front face
        -1.5f, 0.0f, 1.5f,    0.0f, 0.0f, 1.0f,  m_Color.r, m_Color.g, m_Color.b,
         1.5f, 0.0f, 1.5f,    0.0f, 0.0f, 1.0f,  m_Color.r, m_Color.g, m_Color.b,
         1.5f, 8.0f, 1.5f,    0.0f, 0.0f, 1.0f,  m_Color.r, m_Color.g, m_Color.b,
        -1.5f, 8.0f, 1.5f,    0.0f, 0.0f, 1.0f,  m_Color.r, m_Color.g, m_Color.b,
        
        // Back face
        -1.5f, 0.0f, -1.5f,   0.0f, 0.0f, -1.0f, m_Color.r, m_Color.g, m_Color.b,
         1.5f, 0.0f, -1.5f,   0.0f, 0.0f, -1.0f, m_Color.r, m_Color.g, m_Color.b,
         1.5f, 8.0f, -1.5f,   0.0f, 0.0f, -1.0f, m_Color.r, m_Color.g, m_Color.b,
        -1.5f, 8.0f, -1.5f,   0.0f, 0.0f, -1.0f, m_Color.r, m_Color.g, m_Color.b,
        
        // Left face
        -1.5f, 0.0f, -1.5f,   -1.0f, 0.0f, 0.0f, m_Color.r, m_Color.g, m_Color.b,
        -1.5f, 0.0f, 1.5f,    -1.0f, 0.0f, 0.0f, m_Color.r, m_Color.g, m_Color.b,
        -1.5f, 8.0f, 1.5f,    -1.0f, 0.0f, 0.0f, m_Color.r, m_Color.g, m_Color.b,
        -1.5f, 8.0f, -1.5f,   -1.0f, 0.0f, 0.0f, m_Color.r, m_Color.g, m_Color.b,
        
        // Right face
         1.5f, 0.0f, -1.5f,   1.0f, 0.0f, 0.0f,  m_Color.r, m_Color.g, m_Color.b,
         1.5f, 0.0f, 1.5f,    1.0f, 0.0f, 0.0f,  m_Color.r, m_Color.g, m_Color.b,
         1.5f, 8.0f, 1.5f,    1.0f, 0.0f, 0.0f,  m_Color.r, m_Color.g, m_Color.b,
         1.5f, 8.0f, -1.5f,   1.0f, 0.0f, 0.0f,  m_Color.r, m_Color.g, m_Color.b,
        
        // Top face
        -1.5f, 8.0f, -1.5f,   0.0f, 1.0f, 0.0f,  m_Color.r, m_Color.g, m_Color.b,
        -1.5f, 8.0f, 1.5f,    0.0f, 1.0f, 0.0f,  m_Color.r, m_Color.g, m_Color.b,
         1.5f, 8.0f, 1.5f,    0.0f, 1.0f, 0.0f,  m_Color.r, m_Color.g, m_Color.b,
         1.5f, 8.0f, -1.5f,   0.0f, 1.0f, 0.0f,  m_Color.r, m_Color.g, m_Color.b,
        
        // Bottom face
        -1.5f, 0.0f, -1.5f,   0.0f, -1.0f, 0.0f, m_Color.r, m_Color.g, m_Color.b,
        -1.5f, 0.0f, 1.5f,    0.0f, -1.0f, 0.0f, m_Color.r, m_Color.g, m_Color.b,
         1.5f, 0.0f, 1.5f,    0.0f, -1.0f, 0.0f, m_Color.r, m_Color.g, m_Color.b,
         1.5f, 0.0f, -1.5f,   0.0f, -1.0f, 0.0f, m_Color.r, m_Color.g, m_Color.b
    };
    
    // Indices for drawing the triangles
    std::vector<unsigned int> indices = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Left face
        8, 9, 10, 10, 11, 8,
        // Right face
        12, 13, 14, 14, 15, 12,
        // Top face
        16, 17, 18, 18, 19, 16,
        // Bottom face
        20, 21, 22, 22, 23, 20
    };
    
    // Create the mesh using make_unique instead of raw new
    m_Mesh = std::make_unique<Mesh>();
    // Stride of 9: (3 position + 3 normal + 3 color)
    // Position at offset 0, normals at offset 3, colors at offset 6
    m_Mesh->SetVertexData(vertices, indices, 9, 0, 3, -1, 6);
    
    if (!m_Mesh) {
        std::cerr << "Failed to create player mesh!" << std::endl;
        return false;
    }
    
    std::cout << "Player initialized successfully." << std::endl;
    return true;
}

void Player::Update(float deltaTime, const Platform* platform, World* world) {
    // Get the camera for determining movement direction
    Camera* camera = m_Renderer->GetCamera();
    if (!camera || !platform || !world) {
        return;
    }
    
    // Get the camera forward vector (ignoring Y component for XZ-plane movement)
    glm::vec3 forward = camera->GetFront();
    forward.y = 0.0f;
    forward = glm::normalize(forward);
    
    // Get the camera right vector
    glm::vec3 right = camera->GetRight();
    right.y = 0.0f;
    right = glm::normalize(right);
    
    // Movement based on WASD keys
    glm::vec3 moveDirection(0.0f);
    
    if (platform->IsKeyPressed(GLFW_KEY_W)) {
        moveDirection += forward;
    }
    if (platform->IsKeyPressed(GLFW_KEY_S)) {
        moveDirection -= forward;
    }
    if (platform->IsKeyPressed(GLFW_KEY_A)) {
        moveDirection -= right;
    }
    if (platform->IsKeyPressed(GLFW_KEY_D)) {
        moveDirection += right;
    }
    
    // Normalize the movement direction if needed
    if (glm::length(moveDirection) > 0.01f) {
        moveDirection = glm::normalize(moveDirection);
        
        // Set the player's rotation to face the movement direction
        m_RotationY = atan2(moveDirection.x, moveDirection.z);
    }
    
    // Apply horizontal movement
    m_Velocity.x = moveDirection.x * m_Speed;
    m_Velocity.z = moveDirection.z * m_Speed;
    
    // Apply gravity
    m_Velocity.y -= m_Gravity * deltaTime;
    
    // Check if player is on the ground
    float terrainHeight = world->GetTerrainHeightAt(m_Position.x, m_Position.z);
    m_IsGrounded = (m_Position.y <= terrainHeight + 0.1f);
    
    // If on ground, prevent falling further and allow jumping
    if (m_IsGrounded) {
        m_Position.y = terrainHeight;
        m_Velocity.y = 0.0f;
        
        // Jump if space key is pressed
        if (platform->IsKeyPressed(GLFW_KEY_SPACE)) {
            m_Velocity.y = m_JumpForce;
            m_IsGrounded = false;
        }
    }
    
    // Store current position before movement
    glm::vec3 oldPosition = m_Position;
    
    // Update player position
    m_Position += m_Velocity * deltaTime;
    
    // Check if player is below terrain (likely due to steep slopes or falling)
    terrainHeight = world->GetTerrainHeightAt(m_Position.x, m_Position.z);
    if (m_Position.y < terrainHeight) {
        m_Position.y = terrainHeight;
        m_Velocity.y = 0.0f;
        m_IsGrounded = true;
    }
    
    // Limit player position to world bounds with a small buffer to prevent getting stuck at edges
    const float BUFFER = 0.5f; // Small buffer from the edge
    if (m_Position.x < World::WORLD_MIN_X + BUFFER || m_Position.x > World::WORLD_MAX_X - BUFFER) {
        // Reset X position and velocity (collision)
        m_Position.x = glm::clamp(m_Position.x, World::WORLD_MIN_X + BUFFER, World::WORLD_MAX_X - BUFFER);
        m_Velocity.x = 0.0f;
    }
    
    if (m_Position.z < World::WORLD_MIN_Z + BUFFER || m_Position.z > World::WORLD_MAX_Z - BUFFER) {
        // Reset Z position and velocity (collision)
        m_Position.z = glm::clamp(m_Position.z, World::WORLD_MIN_Z + BUFFER, World::WORLD_MAX_Z - BUFFER);
        m_Velocity.z = 0.0f;
    }
    
    // We no longer need to update the camera position here
    // The World class will update the camera controller with our position
}

void Player::Render() {
    if (!m_Renderer || !m_Mesh || !m_Shader) {
        return;
    }
    
    // Use player shader
    m_Shader->Use();
    
    // Get camera matrices
    Camera* camera = m_Renderer->GetCamera();
    if (!camera) {
        return;
    }
    
    // Set shader uniforms
    glm::mat4 projection = camera->GetProjectionMatrix();
    glm::mat4 view = camera->GetViewMatrix();
    
    m_Shader->SetMat4("projection", projection);
    m_Shader->SetMat4("view", view);
    
    // Create model matrix from player position and rotation
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_Position);
    model = glm::rotate(model, m_RotationY, glm::vec3(0.0f, 1.0f, 0.0f));
    m_Shader->SetMat4("model", model);
    
    // Draw the player mesh
    m_Mesh->Draw();
}

} // namespace Sylva 
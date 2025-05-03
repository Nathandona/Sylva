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

void Player::Update(float deltaTime, const Platform* platform, World* world) {
    // Validate inputs
    if (!platform || !world) {
        return;
    }
    
    // Get the camera forward and right vectors for player movement
    Camera* camera = m_Renderer->GetCamera();
    if (!camera) {
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
        // Stop vertical movement if on ground
        m_Velocity.y = 0.0f;
        m_Position.y = terrainHeight + 0.1f;
        
        // Jump if space is pressed
        if (platform->IsKeyPressed(GLFW_KEY_SPACE)) {
            m_Velocity.y = m_JumpForce;
        }
    }
    
    // Update position based on velocity
    m_Position += m_Velocity * deltaTime;
    
    // Ensure player stays within world bounds
    m_Position.x = std::max(World::WORLD_MIN_X + 1.0f, std::min(m_Position.x, World::WORLD_MAX_X - 1.0f));
    m_Position.z = std::max(World::WORLD_MIN_Z + 1.0f, std::min(m_Position.z, World::WORLD_MAX_Z - 1.0f));
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
    
    // Apply rotation around Y axis
    model = glm::rotate(model, m_RotationY, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Set the model matrix in the shader
    m_Shader->SetMat4("model", model);
    
    // Draw the mesh
    m_Mesh->Draw();
}

} // namespace Sylva 
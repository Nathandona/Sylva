#pragma once

#include <glm/glm.hpp>
#include <memory>
#include "../renderer/Renderer.h"
#include "../renderer/Camera.h"
#include "../platform/Platform.h"

// Forward declarations
namespace Sylva {
    class Renderer;
    class Platform;
    class Camera;
    class World; // Forward declare World
    class Mesh;
    class Shader;
}

namespace Sylva {

class Player {
public:
    Player();
    ~Player();

    // Initialize the player with a simple rectangular mesh
    bool Initialize(Renderer* renderer);
    
    // Update player position and state based on input and physics
    void Update(float deltaTime, const Platform* platform, World* world);
    
    // Render the player
    void Render(Camera* camera);
    
    // Get the player's position
    glm::vec3 GetPosition() const { return m_Position; }
    
    // Set the player's position
    void SetPosition(const glm::vec3& position);
    
    // Get the player's yaw (horizontal rotation)
    float GetYaw() const;

private:
    // Player state
    glm::vec3 m_Position;        // Current position
    glm::vec3 m_Velocity;        // Current velocity
    float m_Yaw;                 // Rotation around Y axis (in degrees)
    
    // Physics parameters
    float m_WalkSpeed;           // Normal movement speed
    float m_RunSpeed;            // Running speed
    float m_TurnSpeed;           // Speed for turning (degrees per second)
    float m_JumpForce;           // Force applied when jumping
    float m_Gravity;             // Downward acceleration
    bool m_IsGrounded;           // Whether player is on ground
    float m_GroundCheckDistance; // Distance for ground check
    
    // Methods for handling different aspects of player behavior
    void HandleInput(float deltaTime, const Platform* platform);
    void ApplyGravity(float deltaTime);
    void CheckGroundCollision(World* world);
    
    // Rendering resources
    Renderer* m_Renderer;        // Pointer to renderer (not owned)
    std::unique_ptr<Mesh> m_Mesh; // Player mesh (now using unique_ptr)
    std::shared_ptr<Shader> m_Shader; // Player shader
    glm::vec3 m_Color;           // Player color
};

} // namespace Sylva 
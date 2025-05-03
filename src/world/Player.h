#pragma once

#include <glm/glm.hpp>
#include <memory>
#include "../renderer/Renderer.h"
#include "../platform/Platform.h"

namespace Sylva {

class Player {
public:
    Player();
    ~Player();

    // Initialize the player with a simple rectangular mesh
    bool Initialize(Renderer* renderer);
    
    // Update player position and state based on input and physics
    void Update(float deltaTime, const Platform* platform, class World* world);
    
    // Render the player
    void Render();
    
    // Get the player's position
    glm::vec3 GetPosition() const { return m_Position; }
    
    // Set the player's position
    void SetPosition(const glm::vec3& position) { m_Position = position; }

private:
    // Player state
    glm::vec3 m_Position;        // Current position
    glm::vec3 m_Velocity;        // Current velocity
    float m_RotationY;           // Rotation around Y axis (in radians)
    
    // Physics parameters
    float m_Speed;               // Movement speed
    float m_JumpForce;           // Force applied when jumping
    float m_Gravity;             // Downward acceleration
    bool m_IsGrounded;           // Whether player is on ground
    
    // Rendering resources
    Renderer* m_Renderer;        // Pointer to renderer (not owned)
    std::unique_ptr<Mesh> m_Mesh; // Player mesh (now using unique_ptr)
    std::shared_ptr<Shader> m_Shader; // Player shader
    glm::vec3 m_Color;           // Player color
};

} // namespace Sylva 
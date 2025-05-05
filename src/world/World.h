#pragma once

#include "Terrain.h"
#include "Player.h"
#include "../renderer/Renderer.h"
#include "../renderer/Camera.h"
#include "../renderer/ResourceManager.h"
#include "../input/InputManager.h"
#include <memory>

namespace Sylva {

class World {
public:
    World();
    ~World();

    // Initialize the world with a flat terrain
    bool Initialize(Renderer* renderer, Platform* platform, InputManager* inputManager);
    
    // Update the world state (will be expanded in future)
    void Update(float deltaTime, Camera* camera);
    
    // Render the world
    void Render(Camera* camera);
    
    // Get the height at a specific world position
    float GetTerrainHeightAt(float x, float z) const;
    
    // Check if world has collision data available
    bool HasCollisionData() const { return true; } // Always true for now, can be enhanced later
    
    // Perform a raycast test against the world geometry
    // Returns true if a collision was found, and stores the hit distance
    bool RaycastTest(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, float& hitDistance);
    
    // Get player reference
    const Player& GetPlayer() const { return m_Player; }
    Player& GetPlayer() { return m_Player; }
    
    // Define world coordinate limits
    static constexpr float WORLD_SIZE = 1000.0f;
    static constexpr float WORLD_MIN_X = -WORLD_SIZE / 2.0f;
    static constexpr float WORLD_MAX_X = WORLD_SIZE / 2.0f;
    static constexpr float WORLD_MIN_Z = -WORLD_SIZE / 2.0f;
    static constexpr float WORLD_MAX_Z = WORLD_SIZE / 2.0f;

private:
    // Reference to the renderer (not owned by World)
    Renderer* m_Renderer;
    
    // Platform reference (not owned by World)
    Platform* m_Platform;
    
    // Input manager reference (not owned by World)
    InputManager* m_InputManager;
    
    // Terrain
    Terrain m_Terrain;
    
    // Player
    Player m_Player;
    
    // Rendering resources
    std::shared_ptr<Shader> m_TerrainShader;
    std::shared_ptr<Texture> m_TerrainTexture;
};

} // namespace Sylva 
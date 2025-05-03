#pragma once

#include "Terrain.h"
#include "Player.h"
#include "../renderer/Renderer.h"
#include "../renderer/Camera.h"
#include "../renderer/ResourceManager.h"
#include <memory>

namespace Sylva {

class World {
public:
    World();
    ~World();

    // Initialize the world with a flat terrain
    bool Initialize(Renderer* renderer, Platform* platform);
    
    // Update the world state (will be expanded in future)
    void Update(float deltaTime, Camera* camera);
    
    // Render the world
    void Render(Camera* camera);
    
    // Get the height at a specific world position
    float GetTerrainHeightAt(float x, float z) const;
    
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
    
    // Terrain
    Terrain m_Terrain;
    
    // Player
    Player m_Player;
    
    // Rendering resources
    std::shared_ptr<Shader> m_TerrainShader;
    std::shared_ptr<Texture> m_TerrainTexture;
};

} // namespace Sylva 
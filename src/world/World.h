#pragma once

#include "Terrain.h"
#include "../renderer/Renderer.h"

namespace Sylva {

class World {
public:
    World();
    ~World();

    // Initialize the world with a flat terrain
    bool Initialize(Renderer* renderer);
    
    // Update the world state (will be expanded in future)
    void Update(float deltaTime);
    
    // Render the world
    void Render();
    
    // Get the height at a specific world position
    float GetTerrainHeightAt(float x, float z) const;
    
    // Define world coordinate limits
    static constexpr float WORLD_SIZE = 1000.0f;
    static constexpr float WORLD_MIN_X = -WORLD_SIZE / 2.0f;
    static constexpr float WORLD_MAX_X = WORLD_SIZE / 2.0f;
    static constexpr float WORLD_MIN_Z = -WORLD_SIZE / 2.0f;
    static constexpr float WORLD_MAX_Z = WORLD_SIZE / 2.0f;

private:
    // Reference to the renderer (not owned by World)
    Renderer* m_Renderer;
    
    // Terrain
    Terrain m_Terrain;
    
    // Rendering resources
    Shader* m_TerrainShader;
    Texture* m_TerrainTexture;
};

} // namespace Sylva 
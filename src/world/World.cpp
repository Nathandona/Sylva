#include "World.h"
#include <iostream>

namespace Sylva {

World::World()
    : m_Renderer(nullptr)
    , m_TerrainShader(nullptr)
    , m_TerrainTexture(nullptr)
{
}

World::~World() {
    // We don't own the renderer or shader/texture resources
    m_Renderer = nullptr;
    m_TerrainShader = nullptr;
    m_TerrainTexture = nullptr;
}

bool World::Initialize(Renderer* renderer) {
    if (!renderer) {
        std::cerr << "Cannot initialize World without a valid Renderer!" << std::endl;
        return false;
    }
    
    m_Renderer = renderer;
    
    // Load terrain shader
    m_TerrainShader = m_Renderer->LoadShader("assets/shaders/terrain_vertex.glsl", 
                                             "assets/shaders/terrain_fragment.glsl");
    if (!m_TerrainShader) {
        std::cerr << "Failed to load terrain shader!" << std::endl;
        // For MVP, we'll fall back to using a basic shader if the terrain-specific one fails
        m_TerrainShader = m_Renderer->LoadShader("assets/shaders/basic_vertex.glsl", 
                                                "assets/shaders/basic_fragment.glsl");
        if (!m_TerrainShader) {
            std::cerr << "Failed to load fallback shader!" << std::endl;
            return false;
        }
    }
    
    // Load terrain texture
    m_TerrainTexture = m_Renderer->LoadTexture("assets/textures/terrain_grass.png");
    if (!m_TerrainTexture) {
        std::cerr << "Failed to load terrain texture, using default texture." << std::endl;
        // We'll continue without a texture for now
    }
    
    // Initialize the terrain (100x100 units, 64x64 resolution)
    if (!m_Terrain.Initialize(100.0f, 100.0f, 64)) {
        std::cerr << "Failed to initialize terrain!" << std::endl;
        return false;
    }
    
    // Set the terrain texture if loaded
    if (m_TerrainTexture) {
        m_Terrain.SetTexture(m_TerrainTexture);
    }
    
    std::cout << "World initialized successfully." << std::endl;
    return true;
}

void World::Update(float deltaTime) {
    // Currently empty - will be expanded in the future with:
    // - Entity updates
    // - Chunk loading/unloading
    // - World physics simulation
}

void World::Render() {
    if (!m_Renderer || !m_TerrainShader) {
        std::cerr << "Cannot render World without valid Renderer and Shader!" << std::endl;
        return;
    }
    
    // Render the terrain
    m_Terrain.Render(m_TerrainShader);
    
    // Future: render world entities, effects, etc.
}

float World::GetTerrainHeightAt(float x, float z) const {
    return m_Terrain.GetHeightAt(x, z);
}

} // namespace Sylva 
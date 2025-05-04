#include "World.h"
#include <iostream>

namespace Sylva {

World::World()
    : m_Renderer(nullptr)
    , m_Platform(nullptr)
    , m_InputManager(nullptr)
    , m_TerrainShader(nullptr)
    , m_TerrainTexture(nullptr)
{
}

World::~World() {
    // We don't own these pointers
    m_Renderer = nullptr;
    m_Platform = nullptr;
    m_InputManager = nullptr;
    
    // Shared resources are automatically cleaned up
}

bool World::Initialize(Renderer* renderer, Platform* platform, InputManager* inputManager) {
    if (!renderer) {
        std::cerr << "Cannot initialize World without a valid Renderer!" << std::endl;
        return false;
    }
    
    m_Renderer = renderer;
    m_Platform = platform;
    m_InputManager = inputManager;
    
    if (!m_Platform) {
        std::cerr << "Warning: Platform not provided, input will not be processed" << std::endl;
    }
    
    if (!m_InputManager) {
        std::cerr << "Warning: InputManager not provided, input abstraction will not be available" << std::endl;
    }
    
    // Get resource manager from renderer
    ResourceManager* resourceManager = m_Renderer->GetResourceManager();
    if (!resourceManager) {
        std::cerr << "Error: Renderer does not have a ResourceManager!" << std::endl;
        return false;
    }
    
    // Load terrain shader
    m_TerrainShader = resourceManager->GetShader("assets/shaders/terrain_vertex.glsl", 
                                               "assets/shaders/terrain_fragment.glsl");
    if (!m_TerrainShader) {
        std::cerr << "Failed to load terrain shader!" << std::endl;
        // For MVP, we'll fall back to using a basic shader if the terrain-specific one fails
        m_TerrainShader = resourceManager->GetShader("assets/shaders/basic_vertex.glsl", 
                                                   "assets/shaders/basic_fragment.glsl");
        if (!m_TerrainShader) {
            std::cerr << "Failed to load fallback shader!" << std::endl;
            return false;
        }
    }
    
    // Load terrain texture
    m_TerrainTexture = resourceManager->GetTexture("assets/textures/terrain_grass.png");
    if (!m_TerrainTexture) {
        std::cerr << "Failed to load terrain texture, using default texture." << std::endl;
        // We'll continue without a texture for now
    } else {
        std::cout << "Successfully loaded terrain texture." << std::endl;
    }
    
    // Initialize the terrain (100x100 units, 64x64 resolution)
    if (!m_Terrain.Initialize(100.0f, 100.0f, 64)) {
        std::cerr << "Failed to initialize terrain!" << std::endl;
        return false;
    }
    
    // Set the terrain texture if loaded
    if (m_TerrainTexture) {
        m_Terrain.SetTexture(m_TerrainTexture.get());
    }
    
    // Initialize the player
    if (!m_Player.Initialize(m_Renderer)) {
        std::cerr << "Failed to initialize player!" << std::endl;
        return false;
    }
    
    // Place the player at a reasonable starting position
    float initialHeight = m_Terrain.GetHeightAt(0.0f, 0.0f) + 15.0f; // Start much higher for visibility
    m_Player.SetPosition(glm::vec3(0.0f, initialHeight, 0.0f));
    
    std::cout << "World initialized successfully." << std::endl;
    return true;
}

void World::Update(float deltaTime, Camera* camera) {
    // Update player
    if (m_Platform && camera && m_InputManager) {
        // Use the input manager for player updates
        m_Player.Update(deltaTime, m_Platform, this, m_InputManager);
    }
    
    // Future additions:
    // - Entity updates
    // - Chunk loading/unloading
    // - World physics simulation
}

void World::Render(Camera* camera) {
    if (!camera) {
        return;
    }
    
    // Render terrain
    if (m_TerrainShader) {
        m_TerrainShader->Use();
        
        // Set view and projection matrices from camera
        m_TerrainShader->SetMat4("view", camera->GetViewMatrix());
        m_TerrainShader->SetMat4("projection", camera->GetProjectionMatrix());
        
        // Set lighting values if needed
        m_TerrainShader->SetVec3("lightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f)));
        m_TerrainShader->SetVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.9f));
        m_TerrainShader->SetVec3("viewPos", camera->GetPosition());
        
        // Render the terrain
        m_Terrain.Render(m_TerrainShader.get());
    }
    
    // Render player
    m_Player.Render(camera);
    
    // Future: render additional world entities, effects, etc.
}

float World::GetTerrainHeightAt(float x, float z) const {
    return m_Terrain.GetHeightAt(x, z);
}

} // namespace Sylva 
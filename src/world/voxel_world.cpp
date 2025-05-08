#include "voxel_world.h"
#include "core/config.h"
#include "core/logger.h"
#include "renderer/camera.h"
#include "renderer/shader.h"
#include "player.h"
#include <glad/glad.h>
#include <glm/gtc/noise.hpp>
#include <algorithm>
#include <cmath>
#include <random>
#include "world/terrain_generator.h"

namespace Sylva {

VoxelWorld::VoxelWorld() 
    : m_shader(nullptr) {
    // Load world parameters from config if available
    m_params.terrainSize = Config::getInt("World.terrain_size", m_params.terrainSize);
    m_params.cellSize = Config::getFloat("World.cell_size", m_params.cellSize);
    m_params.maxHeight = Config::getFloat("World.max_height", m_params.maxHeight);
    m_params.seed = Config::getInt("World.seed", m_params.seed);
    m_worldSizeInChunks = Config::getInt("World.size_in_chunks", m_worldSizeInChunks);
    m_viewDistanceInChunks = Config::getInt("World.view_distance", m_viewDistanceInChunks);
    
    m_params.noiseScale = Config::getFloat("World.noise_scale", 0.05f);  
    m_params.detailScale = Config::getFloat("World.detail_scale", 0.20f); 
    m_params.terrainSmoothness = Config::getFloat("World.terrain_smoothness", 0.6f);
    
    m_terrainGenerator = std::make_unique<TerrainGenerator>(m_params);
    
    Logger::logInfo("Micro-voxel world created with size " + std::to_string(m_worldSizeInChunks) + 
                   " chunks and view distance " + std::to_string(m_viewDistanceInChunks) + " chunks");
}

VoxelWorld::VoxelWorld(const WorldParams& params) 
    : m_params(params),
      m_shader(nullptr) {
    m_worldSizeInChunks = Config::getInt("World.size_in_chunks", m_worldSizeInChunks);
    m_viewDistanceInChunks = Config::getInt("World.view_distance", m_viewDistanceInChunks);
    m_terrainGenerator = std::make_unique<TerrainGenerator>(m_params);
    
    Logger::logInfo("VoxelWorld created with custom parameters");
}

VoxelWorld::~VoxelWorld() {
    if (m_shader != nullptr) {
        delete m_shader;
        m_shader = nullptr;
    }
    
    // Clean up debug resources
    if (m_debugVAO != 0) {
        glDeleteVertexArrays(1, &m_debugVAO);
        m_debugVAO = 0;
    }
    
    if (m_debugVBO != 0) {
        glDeleteBuffers(1, &m_debugVBO);
        m_debugVBO = 0;
    }
    
    m_chunks.clear();
}

bool VoxelWorld::initializeGraphics() {
    Logger::logInfo("Initializing micro-voxel world graphics");
    
    // Initialize block data
    BlockData::initialize();
    
    // Create shader
    return createShader();
}

bool VoxelWorld::createShader() {
    // Clean up old shader if it exists
    if (m_shader != nullptr) {
        delete m_shader;
        m_shader = nullptr;
    }
    
    // Create new shader
    try {
        m_shader = new Shader("assets/shaders/voxel.vert", "assets/shaders/voxel.frag");
        Logger::logInfo("Voxel shader created successfully");
        return true;
    } catch (const std::exception& e) {
        Logger::logError("Failed to create voxel shader: " + std::string(e.what()));
        return false;
    }
}

void VoxelWorld::generateWorld(const glm::vec3& playerPosition) {
    Logger::logInfo("Generating micro-voxel world around player position (" + 
                   std::to_string(playerPosition.x) + ", " +
                   std::to_string(playerPosition.y) + ", " +
                   std::to_string(playerPosition.z) + ")");
    
    glm::ivec3 centerChunkPos = Chunk::worldToChunkPos(playerPosition);
    
    for (int y = -1; y <= 7; y++) {
        for (int z = -m_worldSizeInChunks; z <= m_worldSizeInChunks; z++) {
            for (int x = -m_worldSizeInChunks; x <= m_worldSizeInChunks; x++) {
                glm::ivec3 chunkPos = centerChunkPos + glm::ivec3(x, y, z);
                Chunk* chunk = getChunk(chunkPos);
                if (chunk == nullptr) {
                    chunk = createChunk(chunkPos);
                    m_terrainGenerator->generateTerrain(chunk);
                    m_terrainGenerator->generateFeatures(chunk);
                }
            }
        }
    }
    updateChunkMeshes(centerChunkPos);
    Logger::logInfo("Micro-voxel world generation complete");
}

void VoxelWorld::update(float deltaTime, const glm::vec3& playerPosition) {
    glm::ivec3 playerChunkPos = Chunk::worldToChunkPos(playerPosition);
    static glm::ivec3 lastPlayerChunkPos(-999999, -999999, -999999);
    
    if (playerChunkPos != lastPlayerChunkPos) {
        for (int y = -1; y <= 7; y++) {
            for (int z = -m_viewDistanceInChunks; z <= m_viewDistanceInChunks; z++) {
                for (int x = -m_viewDistanceInChunks; x <= m_viewDistanceInChunks; x++) {
                    glm::ivec3 chunkPos = playerChunkPos + glm::ivec3(x, y, z);
                    float distance = sqrt(x*x + z*z); // Simple distance check
                    if (distance <= m_viewDistanceInChunks) {
                        Chunk* chunk = getChunk(chunkPos);
                        if (chunk == nullptr) {
                            chunk = createChunk(chunkPos);
                            m_terrainGenerator->generateTerrain(chunk);
                            m_terrainGenerator->generateFeatures(chunk);
                        }
                    }
                }
            }
        }
        updateChunkMeshes(playerChunkPos);
        lastPlayerChunkPos = playerChunkPos;
    }
}

void VoxelWorld::render(const Camera& camera) {
    if (m_shader == nullptr) {
        Logger::logWarning("Cannot render voxel world, shader not initialized");
        return;
    }
    
    // Get camera matrices
    glm::mat4 viewMatrix = camera.getViewMatrix();
    glm::mat4 projectionMatrix = camera.getProjectionMatrix(16.0f / 9.0f); // TODO: Get actual aspect ratio
    
    // Set up directional light for "Pelican Harbor" stylized aesthetic
    // Warm, slightly angled sunlight creates soft shadows and highlights
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    
    // Activate shader before setting uniforms
    m_shader->use();
    
    // Pass light direction to shader
    m_shader->setVec3("lightDir", lightDir);

    // Get lighting colors from config or use defaults
    glm::vec3 lightColor(
        Config::getFloat("Lighting.light_color.x", 1.0f),
        Config::getFloat("Lighting.light_color.y", 0.95f),
        Config::getFloat("Lighting.light_color.z", 0.8f)
    ); // Warm white
    glm::vec3 ambientColor(
        Config::getFloat("Lighting.ambient_color.x", 0.45f), // Slightly warmer and less blue
        Config::getFloat("Lighting.ambient_color.y", 0.45f),
        Config::getFloat("Lighting.ambient_color.z", 0.45f) 
    ); // Neutral warmish ambient, slightly reduced from a bluish 0.5 on Z

    m_shader->setVec3("uniformLightColor", lightColor);
    m_shader->setVec3("uniformAmbientColor", ambientColor);
    
    // Render each chunk
    for (const auto& pair : m_chunks) {
        pair.second->render(m_shader, viewMatrix, projectionMatrix);
    }
    
    Logger::logDebug("Voxel world rendered with " + std::to_string(m_chunks.size()) + " chunks");
}

Chunk* VoxelWorld::getChunk(const glm::ivec3& chunkPos) const {
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

Chunk* VoxelWorld::createChunk(const glm::ivec3& chunkPos) {
    // Create new chunk
    auto chunk = std::make_unique<Chunk>(chunkPos);
    
    // Store in map and return raw pointer
    Chunk* chunkPtr = chunk.get();
    m_chunks[chunkPos] = std::move(chunk);
    
    // When a new chunk is created, its direct neighbors might need their meshes updated
    // because they now have a new adjacent chunk where there was none (or assumed air).
    const glm::ivec3 neighborOffsets[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };
    for (const auto& offset : neighborOffsets) {
        Chunk* neighbor = getChunk(chunkPos + offset);
        if (neighbor) {
            neighbor->markModified();
        }
    }
    
    return chunkPtr;
}

void VoxelWorld::updateChunkMeshes(const glm::ivec3& centerPos) {
    // For each chunk in the view distance
    for (auto& pair : m_chunks) {
        const glm::ivec3& chunkPos = pair.first;
        Chunk* chunk = pair.second.get();
        
        // Calculate distance from center
        glm::ivec3 diff = chunkPos - centerPos;
        float distance = sqrt(diff.x * diff.x + diff.z * diff.z);
        
        // Skip chunks outside view distance
        if (distance > m_viewDistanceInChunks) {
            continue;
        }
        
        // Get pointers to the 6 surrounding chunks (+X, -X, +Y, -Y, +Z, -Z)
        const Chunk* surroundingChunks[6] = {
            getChunk(chunkPos + glm::ivec3(1, 0, 0)),
            getChunk(chunkPos + glm::ivec3(-1, 0, 0)),
            getChunk(chunkPos + glm::ivec3(0, 1, 0)),
            getChunk(chunkPos + glm::ivec3(0, -1, 0)),
            getChunk(chunkPos + glm::ivec3(0, 0, 1)),
            getChunk(chunkPos + glm::ivec3(0, 0, -1))
        };
        
        // Generate mesh for this chunk
        chunk->generateMesh(surroundingChunks);
    }
}

BlockType VoxelWorld::getBlockAt(const glm::vec3& worldPos) const {
    // Convert to chunk and local coordinates
    glm::ivec3 chunkPos = Chunk::worldToChunkPos(worldPos);
    glm::ivec3 localPos = Chunk::worldToLocalPos(worldPos);
    
    // Get the chunk
    Chunk* chunk = getChunk(chunkPos);
    if (chunk == nullptr) {
        return BlockType::AIR; // Assume air for non-existent chunks
    }
    
    // Get the block from the chunk
    return chunk->getBlock(localPos.x, localPos.y, localPos.z);
}

void VoxelWorld::setBlockAt(const glm::vec3& worldPos, BlockType type) {
    // Convert to chunk and local coordinates
    glm::ivec3 chunkPos = Chunk::worldToChunkPos(worldPos);
    glm::ivec3 localPos = Chunk::worldToLocalPos(worldPos);
    
    // Get or create the chunk
    Chunk* chunk = getChunk(chunkPos);
    if (chunk == nullptr) {
        chunk = createChunk(chunkPos);
    }
    
    // Set the block in the chunk
    chunk->setBlock(localPos.x, localPos.y, localPos.z, type);
    
    // Check if the modified block is on a boundary of the chunk.
    // If so, the neighboring chunk also needs its mesh updated.
    const glm::ivec3 neighborOffsets[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };
    bool onBoundary = false;
    int boundaryDirection = -1;

    if (localPos.x == 0) { onBoundary = true; boundaryDirection = 1; } // Touches -X neighbor
    else if (localPos.x == CHUNK_SIZE - 1) { onBoundary = true; boundaryDirection = 0; } // Touches +X neighbor
    if (localPos.y == 0) { onBoundary = true; boundaryDirection = 3; } // Touches -Y neighbor (can overwrite if multiple boundaries touched)
    else if (localPos.y == CHUNK_SIZE - 1) { onBoundary = true; boundaryDirection = 2; } // Touches +Y neighbor
    if (localPos.z == 0) { onBoundary = true; boundaryDirection = 5; } // Touches -Z neighbor
    else if (localPos.z == CHUNK_SIZE - 1) { onBoundary = true; boundaryDirection = 4; } // Touches +Z neighbor
    
    // A single block can be on multiple boundaries. We need to check all relevant neighbors.
    if (localPos.x == 0) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[1]); // -X neighbor
        if (neighbor) neighbor->markModified();
    }
    if (localPos.x == CHUNK_SIZE - 1) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[0]); // +X neighbor
        if (neighbor) neighbor->markModified();
    }
    if (localPos.y == 0) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[3]); // -Y neighbor
        if (neighbor) neighbor->markModified();
    }
    if (localPos.y == CHUNK_SIZE - 1) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[2]); // +Y neighbor
        if (neighbor) neighbor->markModified();
    }
    if (localPos.z == 0) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[5]); // -Z neighbor
        if (neighbor) neighbor->markModified();
    }
    if (localPos.z == CHUNK_SIZE - 1) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[4]); // +Z neighbor
        if (neighbor) neighbor->markModified();
    }

    // No need to call updateChunkMeshes here directly. 
    // The main loop's call to updateChunkMeshes will handle it because chunks are marked modified.
}

float VoxelWorld::getHeightAt(float x, float z) const {
    // Use higher precision for micro-voxel terrain
    // Start from a high position and move down with smaller steps
    float maxHeight = 128.0f; // Maximum world height in blocks
    float heightIncrement = m_params.cellSize; // Step down by one micro-voxel at a time
    
    for (float y = maxHeight; y >= 0.0f; y -= heightIncrement) {
        glm::vec3 pos(x, y, z);
        BlockType block = getBlockAt(pos);
        
        if (block != BlockType::AIR && BlockData::isSolid(block)) {
            // Return the height of the block top
            // Add cell size to get the top surface
            return y + m_params.cellSize;
        }
    }
    
    // Fallback to 0 if no solid block found
    return 0.0f;
}

bool VoxelWorld::checkCollision(const Player& player) const {
    // Get player position and size
    Vec3 playerPos = player.getPosition();
    float playerSize = player.getCollisionRadius();
    
    // Clear previous debug points if debug is enabled
    if (m_collisionDebugEnabled) {
        m_collisionDebugPoints.clear();
    }
    
    // Calculate step size based on cell size for proper micro-voxel sampling
    // Use a fraction of cellSize to ensure thorough collision checking
    float stepSize = m_params.cellSize * 0.5f;
    
    // Ensure we have a reasonable step size (not too small, not too large)
    stepSize = std::max(stepSize, 0.05f);
    
    bool collision = false;
    
    // Check blocks in player's bounding box with appropriate sampling density
    for (float y = playerPos.y; y <= playerPos.y + player.getHeight(); y += stepSize) {
        for (float z = playerPos.z - playerSize; z <= playerPos.z + playerSize; z += stepSize) {
            for (float x = playerPos.x - playerSize; x <= playerPos.x + playerSize; x += stepSize) {
                BlockType block = getBlockAt(glm::vec3(x, y, z));
                
                if (block != BlockType::AIR && BlockData::isSolid(block)) {
                    // Store collision point for debug visualization
                    if (m_collisionDebugEnabled) {
                        m_collisionDebugPoints.push_back(glm::vec3(x, y, z));
                    }
                    
                    collision = true;
                    
                    // If not in debug mode, return immediately on first collision
                    if (!m_collisionDebugEnabled) {
                        return true;
                    }
                }
            }
        }
    }
    
    return collision;
}

void VoxelWorld::setWorldSize(int size) {
    m_worldSizeInChunks = size;
}

void VoxelWorld::renderCollisionDebug(const Camera& camera, const Player& player) {
    if (!m_collisionDebugEnabled || m_collisionDebugPoints.empty()) {
        return;
    }
    
    // Check if we need to initialize debug rendering resources
    if (m_debugVAO == 0) {
        glGenVertexArrays(1, &m_debugVAO);
        glGenBuffers(1, &m_debugVBO);
        
        glBindVertexArray(m_debugVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_debugVBO);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    // Create a simple shader program for rendering debug points if m_shader is not available
    static Shader* debugShader = nullptr;
    if (debugShader == nullptr) {
        try {
            debugShader = new Shader("assets/shaders/debug.vert", "assets/shaders/debug.frag");
        } catch (const std::exception& e) {
            Logger::logError("Failed to create debug shader: " + std::string(e.what()));
            return;
        }
    }
    
    // Update buffer with collision points
    glBindBuffer(GL_ARRAY_BUFFER, m_debugVBO);
    glBufferData(GL_ARRAY_BUFFER, m_collisionDebugPoints.size() * sizeof(glm::vec3), 
                m_collisionDebugPoints.data(), GL_DYNAMIC_DRAW);
    
    // Render points
    debugShader->use();
    debugShader->setMat4("view", camera.getViewMatrix());
    debugShader->setMat4("projection", camera.getProjectionMatrix(16.0f / 9.0f)); // TODO: Get actual aspect ratio
    debugShader->setVec4("color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red points
    
    // Point size for visibility, configurable
    float pointSize = Config::getFloat("Debug.collision_point_size", 5.0f);
    glPointSize(pointSize);
    
    // Disable depth test temporarily to make points visible through blocks
    GLboolean depthEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
    glDisable(GL_DEPTH_TEST);
    
    glBindVertexArray(m_debugVAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_collisionDebugPoints.size()));
    glBindVertexArray(0);
    
    // Restore depth test state
    if (depthEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
    
    // Reset point size
    glPointSize(1.0f);
}

} // namespace Sylva 
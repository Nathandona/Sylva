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
#include "generation/terrain_generator.h"

namespace Sylva {

VoxelWorld::VoxelWorld() {
    // Load world parameters from config if available
    m_params.terrainSize = Config::getInt("World.terrain_size", m_params.terrainSize);
    m_params.cellSize = Config::getFloat("World.cell_size", m_params.cellSize);
    m_params.maxHeight = Config::getFloat("World.max_height", m_params.maxHeight);
    m_params.seed = Config::getInt("World.seed", m_params.seed);
    m_worldSizeInChunks = Config::getInt("World.size_in_chunks", m_worldSizeInChunks);
    m_viewDistanceInChunks = Config::getInt("World.view_distance", m_viewDistanceInChunks);
    m_chunkYMin = Config::getInt("World.chunk_y_min", m_chunkYMin);
    m_chunkYMax = Config::getInt("World.chunk_y_max", m_chunkYMax);

    m_params.noiseScale = Config::getFloat("World.noise_scale", 0.05f);
    m_params.detailScale = Config::getFloat("World.detail_scale", 0.20f);
    m_params.terrainSmoothness = Config::getFloat("World.terrain_smoothness", 0.6f);
    
    m_terrainGenerator = std::make_unique<TerrainGenerator>(m_params);

    Logger::logInfo("Micro-voxel world created with size " + std::to_string(m_worldSizeInChunks) +
                   " chunks and view distance " + std::to_string(m_viewDistanceInChunks) + " chunks");
}

VoxelWorld::VoxelWorld(const WorldParams& params)
    : m_params(params) {
    m_worldSizeInChunks = Config::getInt("World.size_in_chunks", m_worldSizeInChunks);
    m_viewDistanceInChunks = Config::getInt("World.view_distance", m_viewDistanceInChunks);
    m_chunkYMin = Config::getInt("World.chunk_y_min", m_chunkYMin);
    m_chunkYMax = Config::getInt("World.chunk_y_max", m_chunkYMax);
    m_terrainGenerator = std::make_unique<TerrainGenerator>(m_params);

    Logger::logInfo("VoxelWorld created with custom parameters");
}

VoxelWorld::~VoxelWorld() {
    if (m_debugVAO != 0) {
        glDeleteVertexArrays(1, &m_debugVAO);
        m_debugVAO = 0;
    }
    if (m_debugVBO != 0) {
        glDeleteBuffers(1, &m_debugVBO);
        m_debugVBO = 0;
    }
    // m_shader / m_debugShader / m_chunks cleaned up by unique_ptr / map dtors.
}

bool VoxelWorld::initializeGraphics() {
    Logger::logInfo("Initializing micro-voxel world graphics");
    
    // Initialize block data
    BlockData::initialize();
    
    // Create shader
    return createShader();
}

bool VoxelWorld::createShader() {
    try {
        m_shader = std::make_unique<Shader>("assets/shaders/voxel.vert", "assets/shaders/voxel.frag");
        Logger::logInfo("Voxel shader created successfully");
        return true;
    } catch (const std::exception& e) {
        Logger::logError("Failed to create voxel shader: " + std::string(e.what()));
        return false;
    }
}

void VoxelWorld::initializeWorldChunks(const glm::ivec3& centerPos) {
    for (int y = m_chunkYMin; y <= m_chunkYMax; y++) {
        for (int z = -m_worldSizeInChunks; z <= m_worldSizeInChunks; z++) {
            for (int x = -m_worldSizeInChunks; x <= m_worldSizeInChunks; x++) {
                loadChunkIfMissing(centerPos + glm::ivec3(x, y, z));
            }
        }
    }
}

void VoxelWorld::loadChunkIfMissing(const glm::ivec3& chunkPos) {
    if (getChunk(chunkPos) != nullptr) return;
    Chunk* chunk = createChunk(chunkPos);
    generateChunkTerrain(chunk);
    generateChunkFeatures(chunk);
}

void VoxelWorld::generateWorld(const glm::vec3& playerPosition) {
    Logger::logInfo("Generating micro-voxel world around player position (" +
                   std::to_string(playerPosition.x) + ", " +
                   std::to_string(playerPosition.y) + ", " +
                   std::to_string(playerPosition.z) + ")");
    glm::ivec3 centerChunkPos = Chunk::worldToChunkPos(playerPosition);
    initializeWorldChunks(centerChunkPos);
    updateChunkMeshes(centerChunkPos);
    Logger::logInfo("Micro-voxel world generation complete");
}

void VoxelWorld::generateChunkTerrain(Chunk* chunk) {
    if (chunk == nullptr) {
        Logger::logWarning("Attempted to generate terrain for null chunk");
        return;
    }
    m_terrainGenerator->generateTerrain(chunk);
}

void VoxelWorld::generateChunkFeatures(Chunk* chunk) {
    if (chunk == nullptr) {
        Logger::logWarning("Attempted to generate features for null chunk");
        return;
    }
    m_terrainGenerator->generateFeatures(chunk);
}

bool VoxelWorld::updateChunkVisibility(const glm::ivec3& chunkPos, const glm::ivec3& playerChunkPos) const {
    glm::ivec3 diff = chunkPos - playerChunkPos;
    float distance = sqrt(diff.x * diff.x + diff.z * diff.z);
    return distance <= m_viewDistanceInChunks;
}

bool VoxelWorld::updatePlayerChunkPosition(const glm::ivec3& playerChunkPos, glm::ivec3& lastPlayerChunkPos) const {
    if (playerChunkPos != lastPlayerChunkPos) {
        lastPlayerChunkPos = playerChunkPos;
        return true;
    }
    return false;
}

void VoxelWorld::updateChunkLoading(const glm::ivec3& playerChunkPos) {
    for (int y = m_chunkYMin; y <= m_chunkYMax; y++) {
        for (int z = -m_viewDistanceInChunks; z <= m_viewDistanceInChunks; z++) {
            for (int x = -m_viewDistanceInChunks; x <= m_viewDistanceInChunks; x++) {
                glm::ivec3 chunkPos = playerChunkPos + glm::ivec3(x, y, z);
                if (updateChunkVisibility(chunkPos, playerChunkPos)) {
                    loadChunkIfMissing(chunkPos);
                }
            }
        }
    }
    updateChunkMeshes(playerChunkPos);
}

void VoxelWorld::update(float deltaTime, const glm::vec3& playerPosition) {
    glm::ivec3 playerChunkPos = Chunk::worldToChunkPos(playerPosition);
    static glm::ivec3 lastPlayerChunkPos(-999999, -999999, -999999);
    
    if (updatePlayerChunkPosition(playerChunkPos, lastPlayerChunkPos)) {
        updateChunkLoading(playerChunkPos);
    }
}

void VoxelWorld::render(const Camera& camera, float aspectRatio) {
    if (!m_shader) {
        Logger::logWarning("Cannot render voxel world, shader not initialized");
        return;
    }
    glm::mat4 viewMatrix = camera.getViewMatrix();
    glm::mat4 projectionMatrix = camera.getProjectionMatrix(aspectRatio);
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    m_shader->use();
    m_shader->setVec3("lightDir", lightDir);
    glm::vec3 lightColor(
        Config::getFloat("Lighting.light_color.x", 1.0f),
        Config::getFloat("Lighting.light_color.y", 0.95f),
        Config::getFloat("Lighting.light_color.z", 0.8f)
    );
    glm::vec3 ambientColor(
        Config::getFloat("Lighting.ambient_color.x", 0.45f),
        Config::getFloat("Lighting.ambient_color.y", 0.45f),
        Config::getFloat("Lighting.ambient_color.z", 0.45f)
    );
    m_shader->setVec3("uniformLightColor", lightColor);
    m_shader->setVec3("uniformAmbientColor", ambientColor);
    for (const auto& pair : m_chunks) {
        pair.second->render(m_shader.get(), viewMatrix, projectionMatrix);
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

    // If the modified block lies on a chunk face, the neighboring chunk's mesh
    // depends on it (visibility / AO). A single block can touch up to three
    // neighbors (corner). Mark each affected neighbor.
    struct Boundary { int axis; bool atEdge; glm::ivec3 offset; };
    const Boundary boundaries[6] = {
        { localPos.x, localPos.x == 0,              {-1, 0, 0} },
        { localPos.x, localPos.x == CHUNK_SIZE - 1, { 1, 0, 0} },
        { localPos.y, localPos.y == 0,              { 0,-1, 0} },
        { localPos.y, localPos.y == CHUNK_SIZE - 1, { 0, 1, 0} },
        { localPos.z, localPos.z == 0,              { 0, 0,-1} },
        { localPos.z, localPos.z == CHUNK_SIZE - 1, { 0, 0, 1} },
    };
    for (const auto& b : boundaries) {
        if (!b.atEdge) continue;
        if (Chunk* neighbor = getChunk(chunkPos + b.offset)) {
            neighbor->markModified();
        }
    }
    // updateChunkMeshes runs from the main loop; marked chunks will re-mesh then.
}

float VoxelWorld::getHeightAt(float x, float z) const {
    const float maxHeight = m_params.maxHeight;
    const float heightIncrement = m_params.cellSize;
    for (float y = maxHeight; y >= 0.0f; y -= heightIncrement) {
        BlockType block = getBlockAt(glm::vec3(x, y, z));
        if (block != BlockType::AIR && BlockData::isSolid(block)) {
            return y + m_params.cellSize;
        }
    }
    return 0.0f;
}

bool VoxelWorld::checkCollision(const Player& player) const {
    Vec3 playerPos = player.getPosition();
    float playerSize = player.getCollisionRadius();
    float stepSize = std::max(m_params.cellSize * 0.5f, 0.05f);
    bool collision = false;
    for (float y = playerPos.y; y <= playerPos.y + player.getHeight(); y += stepSize) {
        for (float z = playerPos.z - playerSize; z <= playerPos.z + playerSize; z += stepSize) {
            for (float x = playerPos.x - playerSize; x <= playerPos.x + playerSize; x += stepSize) {
                BlockType block = getBlockAt(glm::vec3(x, y, z));
                if (block != BlockType::AIR && BlockData::isSolid(block)) {
                    collision = true;
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

void VoxelWorld::renderCollisionDebug(const Camera& camera, const Player& player, float aspectRatio) {
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
    
    // Lazy-init debug shader as a member (lifetime tied to VoxelWorld; no leak).
    if (!m_debugShader) {
        try {
            m_debugShader = std::make_unique<Shader>("assets/shaders/debug.vert", "assets/shaders/debug.frag");
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
    m_debugShader->use();
    m_debugShader->setMat4("view", camera.getViewMatrix());
    m_debugShader->setMat4("projection", camera.getProjectionMatrix(aspectRatio));
    m_debugShader->setVec4("color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red points
    
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
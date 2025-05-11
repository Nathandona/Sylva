#pragma once

#include "chunk.h"
#include "core/types.h"
#include "world/terrain_generator.h"
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <vector>
#include "voxel_world_generation.h"
#include "voxel_world_rendering.h"
#include "voxel_world_physics.h"

namespace Sylva {

// Forward declarations
class Camera;
class Shader;
class Player;
class VoxelWorldRendering;
class VoxelWorldPhysics;

/**
 * @brief Hash function for glm::ivec3 to use in unordered_map
 */
struct Vec3Hash {
    size_t operator()(const glm::ivec3& v) const {
        // Simple hash combining the three components
        return std::hash<int>()(v.x) ^ 
               (std::hash<int>()(v.y) << 1) ^ 
               (std::hash<int>()(v.z) << 2);
    }
};

/**
 * @brief Manages the voxel-based world with chunks
 */
class VoxelWorld {
public:
    /**
     * @brief Constructor
     */
    VoxelWorld();
    
    /**
     * @brief Constructor with parameters
     * @param params World parameters to use
     */
    VoxelWorld(const WorldParams& params);
    
    /**
     * @brief Destructor
     */
    ~VoxelWorld();
    
    /**
     * @brief Initialize world graphics
     * @return true if initialization successful
     */
    bool initializeGraphics();
    
    /**
     * @brief Generate the world
     * @param playerPosition The position to center world generation around
     */
    void generateWorld(const glm::vec3& playerPosition);
    
    /**
     * @brief Update the world
     * @param deltaTime Time since last update
     * @param playerPosition The current player position
     */
    void update(float deltaTime, const glm::vec3& playerPosition);
    
    /**
     * @brief Render the world
     * @param camera The camera to use for rendering
     */
    void render(const Camera& camera);
    
    /**
     * @brief Check collision with a player
     * @param player The player to check collision against
     * @return True if collision detected, false otherwise
     */
    bool checkCollision(const Player& player) const;
    
    /**
     * @brief Get the block type at a specific world position
     * @param worldPos The world position
     * @return The block type at the specified position
     */
    BlockType getBlockAt(const glm::vec3& worldPos) const;
    
    /**
     * @brief Set the block type at a specific world position
     * @param worldPos The world position
     * @param type The new block type
     */
    void setBlockAt(const glm::vec3& worldPos, BlockType type);
    
    /**
     * @brief Get the height of the terrain at a specific position
     * @param x The x-coordinate in world space
     * @param z The z-coordinate in world space
     * @return The height at the specified position
     */
    float getHeightAt(float x, float z) const;
    
    /**
     * @brief Set the terrain size
     * @param size The new terrain size in chunks
     */
    void setWorldSize(int size);
    
    /**
     * @brief Enable or disable collision debug visualization
     * @param enabled Whether collision debug visualization should be enabled
     */
    void setCollisionDebugEnabled(bool enabled) { m_collisionDebugEnabled = enabled; }
    
    /**
     * @brief Check if collision debug visualization is enabled
     * @return True if collision debug is enabled, false otherwise
     */
    bool isCollisionDebugEnabled() const { return m_collisionDebugEnabled; }
    
    /**
     * @brief Debug render method to visualize collision points
     * @param camera The camera to use for rendering
     * @param player The player to check collision points for
     */
    void renderCollisionDebug(const Camera& camera, const Player& player);
    
    /**
     * @brief Initialize chunks around a center position
     * @param centerPos The center position in chunk coordinates
     */
    void initializeWorldChunks(const glm::ivec3& centerPos);

    /**
     * @brief Generate terrain for a specific chunk
     * @param chunk The chunk to generate terrain for
     */
    void generateChunkTerrain(Chunk* chunk);

    /**
     * @brief Generate features for a specific chunk
     * @param chunk The chunk to generate features for
     */
    void generateChunkFeatures(Chunk* chunk);

    /**
     * @brief Update chunk loading based on player position
     * @param playerChunkPos The player's current chunk position
     */
    void updateChunkLoading(const glm::ivec3& playerChunkPos);

    /**
     * @brief Update chunk visibility based on view distance
     * @param chunkPos The chunk position to check
     * @param playerChunkPos The player's current chunk position
     * @return True if the chunk should be visible
     */
    bool updateChunkVisibility(const glm::ivec3& chunkPos, const glm::ivec3& playerChunkPos) const;

    /**
     * @brief Update player chunk position tracking
     * @param playerChunkPos The player's current chunk position
     * @param lastPlayerChunkPos Reference to the last known player chunk position
     * @return True if the player has moved to a new chunk
     */
    bool updatePlayerChunkPosition(const glm::ivec3& playerChunkPos, glm::ivec3& lastPlayerChunkPos) const;

    Shader* getShader() const { return m_shader; }
    const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, Vec3Hash>& getChunks() const { return m_chunks; }
    const WorldParams& getParams() const { return m_params; }

private:
    /**
     * @brief Get a chunk at the specified position
     * @param chunkPos The chunk coordinates
     * @return Pointer to the chunk, or nullptr if not loaded
     */
    Chunk* getChunk(const glm::ivec3& chunkPos) const;
    
    /**
     * @brief Create a chunk at the specified position
     * @param chunkPos The chunk coordinates
     * @return Pointer to the created chunk
     */
    Chunk* createChunk(const glm::ivec3& chunkPos);
    
    /**
     * @brief Update chunk meshes around a position
     * @param centerPos Center position for mesh updates
     */
    void updateChunkMeshes(const glm::ivec3& centerPos);
    
    /**
     * @brief Create a shader for the voxel world
     * @return true if successful, false otherwise
     */
    bool createShader();
    
    // World parameters
    WorldParams m_params;
    
    // Chunks indexed by position
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, Vec3Hash> m_chunks;
    
    // Rendering resources
    Shader* m_shader;
    
    // World generation parameters
    int m_worldSizeInChunks = 8;     // Number of chunks in each direction from center
    int m_viewDistanceInChunks = 6;  // Maximum view distance in chunks
    
    // Store collision points for debug visualization
    mutable std::vector<glm::vec3> m_collisionDebugPoints;
    mutable bool m_collisionDebugEnabled = false;
    unsigned int m_debugVAO = 0;
    unsigned int m_debugVBO = 0;
    
    std::unique_ptr<TerrainGenerator> m_terrainGenerator;
    std::unique_ptr<VoxelWorldGeneration> m_generation;
    std::unique_ptr<VoxelWorldRendering> m_rendering;
    std::unique_ptr<VoxelWorldPhysics> m_physics;
};

} // namespace Sylva 
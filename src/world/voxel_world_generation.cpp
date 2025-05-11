#include "voxel_world_generation.h"
#include "voxel_world.h"
#include "core/logger.h"
#include "world/terrain_generator.h"
#include <glm/gtc/noise.hpp>
#include <string>

namespace Sylva {

VoxelWorldGeneration::VoxelWorldGeneration(TerrainGenerator* terrainGenerator)
    : m_terrainGenerator(terrainGenerator) {}

void VoxelWorldGeneration::generateWorld(VoxelWorld* world, const glm::vec3& playerPosition) {
    Logger::logInfo("Generating micro-voxel world around player position (" + 
                   std::to_string(playerPosition.x) + ", " +
                   std::to_string(playerPosition.y) + ", " +
                   std::to_string(playerPosition.z) + ")");
    
    glm::ivec3 centerChunkPos = Chunk::worldToChunkPos(playerPosition);
    world->initializeWorldChunks(centerChunkPos);
    // updateChunkMeshes is still part of VoxelWorld core logic
    Logger::logInfo("Micro-voxel world generation complete");
}

void VoxelWorldGeneration::generateChunkTerrain(Chunk* chunk) {
    if (chunk == nullptr) {
        Logger::logWarning("Attempted to generate terrain for null chunk");
        return;
    }
    m_terrainGenerator->generateTerrain(chunk);
}

void VoxelWorldGeneration::generateChunkFeatures(Chunk* chunk) {
    if (chunk == nullptr) {
        Logger::logWarning("Attempted to generate features for null chunk");
        return;
    }
    m_terrainGenerator->generateFeatures(chunk);
}

} // namespace Sylva 
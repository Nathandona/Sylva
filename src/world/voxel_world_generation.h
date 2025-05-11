#pragma once

#include "chunk.h"
#include "core/types.h"
#include <glm/glm.hpp>

namespace Sylva {

class VoxelWorldGeneration {
public:
    VoxelWorldGeneration(class TerrainGenerator* terrainGenerator);
    void generateWorld(class VoxelWorld* world, const glm::vec3& playerPosition);
    void generateChunkTerrain(Chunk* chunk);
    void generateChunkFeatures(Chunk* chunk);
private:
    TerrainGenerator* m_terrainGenerator;
};

} // namespace Sylva 
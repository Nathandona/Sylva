#pragma once

#include "core/types.h"
#include "world/chunk.h" // For BlockType and Chunk, might be needed for generation context
#include <glm/glm.hpp>
#include <random> // For std::mt19937

namespace Sylva {

// Forward declaration if VoxelWorld needs to be referenced, though likely not for generator
// class VoxelWorld; 

/**
 * @brief Encapsulates terrain generation logic including noise functions,
 *        biome logic, and feature placement algorithms.
 */
class TerrainGenerator {
public:
    /**
     * @brief Constructor
     * @param params World parameters to use for generation.
     */
    TerrainGenerator(const WorldParams& params);

    /**
     * @brief Generate the base terrain for a given chunk.
     * @param chunk The chunk to generate terrain for.
     */
    void generateTerrain(Chunk* chunk);

    /**
     * @brief Generate features (e.g., trees, rocks) for a given chunk.
     * @param chunk The chunk to add features to.
     */
    void generateFeatures(Chunk* chunk);

    /**
     * @brief Get the height of the terrain at a specific world position.
     *        This might be complex if it depends on detailed voxel data not yet generated.
     *        For now, assuming it's based on the primary noise functions.
     * @param x The x-coordinate in world space.
     * @param z The z-coordinate in world space.
     * @return The height at the specified position.
     */
    float getHeightAt(float x, float z) const;

private:
    // Helper method for Perlin noise, could be more complex
    float getPerlinNoise(float x, float y, float scale, int octaves, float persistence, float lacunarity) const;
    
    // Helper to get height at a specific integer voxel column
    int getColumnHeight(int worldX, int worldZ) const;


    WorldParams m_params; // Store world generation parameters
    // Add any other members needed for generation, e.g., noise instances if they have state
};

} // namespace Sylva 
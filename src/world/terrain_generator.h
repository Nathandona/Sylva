#pragma once

#include "core/types.h"
#include "world/chunk/chunk.h"
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
    /**
     * @brief Generate base height for a given world position
     * @param worldX X coordinate in world space
     * @param worldZ Z coordinate in world space
     * @param rng Random number generator for noise
     * @return Base height value before biome modifications
     */
    float generateBaseHeight(float worldX, float worldZ, std::mt19937& rng) const;

    /**
     * @brief Generate biome data for a given world position
     * @param worldX X coordinate in world space
     * @param worldZ Z coordinate in world space
     * @param rng Random number generator for noise
     * @return Pair of humidity and temperature values
     */
    std::pair<float, float> generateBiomeData(float worldX, float worldZ, std::mt19937& rng) const;

    /**
     * @brief Apply terrain features to modify base height
     * @param baseHeight Initial height value
     * @param worldX X coordinate in world space
     * @param worldZ Z coordinate in world space
     * @param rng Random number generator for noise
     * @return Modified height value
     */
    float applyTerrainFeatures(float baseHeight, float worldX, float worldZ, std::mt19937& rng) const;

    /**
     * @brief Apply environmental effects based on biome data
     * @param chunk Target chunk
     * @param x_local Local X coordinate in chunk
     * @param z_local Local Z coordinate in chunk
     * @param height Terrain height
     * @param humidity Biome humidity value
     * @param temperature Biome temperature value
     */
    void applyEnvironmentalEffects(Chunk* chunk, int x_local, int z_local, float height, float humidity, float temperature) const;

    // Feature generation helpers
    /**
     * @brief Generate trees in the chunk
     * @param chunk Target chunk
     * @param rng Random number generator
     * @param randPos Random position distribution
     */
    void generateTrees(Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos);

    /**
     * @brief Generate rocks in the chunk
     * @param chunk Target chunk
     * @param rng Random number generator
     * @param randPos Random position distribution
     */
    void generateRocks(Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos);

    /**
     * @brief Generate vegetation (flowers) in the chunk
     * @param chunk Target chunk
     * @param rng Random number generator
     * @param randPos Random position distribution
     */
    void generateVegetation(Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos);

    /**
     * @brief Generate decorations (mushrooms and bushes) in the chunk
     * @param chunk Target chunk
     * @param rng Random number generator
     * @param randPos Random position distribution
     */
    void generateDecorations(Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos);

    // Helper method for Perlin noise, could be more complex
    float getPerlinNoise(float x, float y, float scale, int octaves, float persistence, float lacunarity) const;
    
    // Helper to get height at a specific integer voxel column
    int getColumnHeight(int worldX, int worldZ) const;

    // Store world generation parameters
    WorldParams m_params;
};

} // namespace Sylva 
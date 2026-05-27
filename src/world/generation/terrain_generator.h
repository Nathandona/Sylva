#pragma once

#include "core/types.h"
#include "world/chunk/chunk.h"
#include "biome_generator.h"
#include "feature_generator.h"
#include <glm/glm.hpp>
#include <random> // For std::mt19937
#include <memory>

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
     * @brief Apply terrain features to modify base height
     * @param baseHeight Initial height value
     * @param worldX X coordinate in world space
     * @param worldZ Z coordinate in world space
     * @param rng Random number generator for noise
     * @return Modified height value
     */
    float applyTerrainFeatures(float baseHeight, float worldX, float worldZ, std::mt19937& rng) const;

    /**
     * @brief Helper method for Perlin noise, could be more complex
     * @param x X coordinate in world space
     * @param y Y coordinate in world space
     * @param scale Scale factor for noise
     * @param octaves Number of octaves for noise
     * @param persistence Persistence factor for noise
     * @param lacunarity Lacunarity factor for noise
     * @return Perlin noise value
     */
    float getPerlinNoise(float x, float y, float scale, int octaves, float persistence, float lacunarity) const;
    
    /**
     * @brief Helper to get height at a specific integer voxel column
     * @param worldX X coordinate in world space
     * @param worldZ Z coordinate in world space
     * @return Height at the specified column
     */
    int getColumnHeight(int worldX, int worldZ) const;

    // Store world generation parameters
    WorldParams m_params;
    std::unique_ptr<BiomeGenerator> m_biomeGen;
    std::unique_ptr<FeatureGenerator> m_featureGen;
};

} // namespace Sylva 
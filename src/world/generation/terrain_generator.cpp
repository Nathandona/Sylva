#include "terrain_generator.h"
#include "biome_generator.h"
#include "feature_generator.h"
#include "core/logger.h"
#include "world/chunk/chunk.h" // For Chunk, BlockType, BlockData, CHUNK_SIZE
#include "core/config.h"       // If params in TerrainGenerator need defaults from config
#include <glm/gtc/noise.hpp>
#include <cmath>     // For std::pow, std::abs, sin, etc.
#include <algorithm> // For std::max, std::min

namespace Sylva {

TerrainGenerator::TerrainGenerator(const WorldParams& params)
    : m_params(params),
      m_biomeGen(std::make_unique<BiomeGenerator>(params)),
      m_featureGen(std::make_unique<FeatureGenerator>(params)) {
    Logger::logInfo("TerrainGenerator created.");
}

namespace {
// Snap a voxel-space coordinate to the plateau grid (cell origin). All voxel
// columns inside the same plateau cell receive the same noise sample → flat
// plateau, sharp step at cell boundary.
float quantizeToPlateau(float v, int plateauWidth) {
    if (plateauWidth <= 1)
        return v;
    const float w = static_cast<float>(plateauWidth);
    return std::floor(v / w) * w;
}
} // namespace

float TerrainGenerator::generateBaseHeight(float worldX, float worldZ, std::mt19937& /*rng*/) const {
    float const continentalScale = m_params.noiseScale * 0.2f;
    float const continentalNoise = glm::simplex(glm::vec2(worldX * continentalScale, worldZ * continentalScale));

    float const regionalScale = m_params.noiseScale * 0.5f;
    float const regionalNoise = glm::simplex(glm::vec2(worldX * regionalScale, worldZ * regionalScale));

    float const localScale = m_params.noiseScale;
    float const localNoise = glm::simplex(glm::vec2(worldX * localScale, worldZ * localScale));

    // Generate detail noise with multiple octaves
    float detailNoiseVal = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float totalAmplitude = 0.0f;

    for (int octave = 0; octave < 4; octave++) {
        float const octaveScale = m_params.detailScale * frequency;
        detailNoiseVal += amplitude * glm::simplex(glm::vec2(worldX * octaveScale, worldZ * octaveScale));
        totalAmplitude += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    if (totalAmplitude > 0.0f) {
        detailNoiseVal /= totalAmplitude;
    }

    // Default weights for noise combination
    float const continentalWeight = 0.55f;
    float const regionalWeight = 0.30f;
    float const localWeight = 0.12f;
    float const detailWeight = 0.03f;

    // Combine noise values
    float const combinedNoise = (continentalNoise * continentalWeight) + (regionalNoise * regionalWeight) +
                                (localNoise * localWeight) + (detailNoiseVal * detailWeight);

    // Apply base curve factor
    float const curveFactor = sin(worldX * 0.01f) * sin(worldZ * 0.01f) * 0.1f + 0.9f;
    float const height = ((combinedNoise + 1.0f) * 0.5f * m_params.maxHeight) * curveFactor;

    return height;
}

float TerrainGenerator::applyTerrainFeatures(float baseHeight, float worldX, float worldZ, std::mt19937& /*rng*/) const {
    float height = baseHeight;

    // Apply additional terrain features
    float const featureNoise = glm::simplex(glm::vec2(worldX * 0.05f, worldZ * 0.05f));
    if (featureNoise > 0.7f) {
        height += featureNoise * 3.0f;
    } else if (featureNoise < -0.7f) {
        height -= fabsf(featureNoise) * 2.0f;
    }

    // Ensure minimum height
    height = std::max(height, 5.0f);

    return height;
}

void TerrainGenerator::generateTerrain(Chunk* chunk) {
    if (chunk == nullptr) {
        return;
    }

    const glm::ivec3& chunkPos = chunk->getPosition();
    int const chunkSeed = m_params.seed + chunkPos.x * 10000 + chunkPos.y * 1000 + chunkPos.z * 100;
    std::mt19937 rng(chunkSeed);

    const int plateauW = m_params.plateauWidthVoxels;
    for (int z_local = 0; z_local < CHUNK_SIZE; z_local++) {
        for (int x_local = 0; x_local < CHUNK_SIZE; x_local++) {
            const float worldX = (chunkPos.x * CHUNK_SIZE) + x_local;
            const float worldZ = (chunkPos.z * CHUNK_SIZE) + z_local;
            // Sample noise on plateau grid so every column inside a cell shares
            // a height — produces flat walkable steps instead of voxel-scale
            // ripples. Biome humidity/temperature stay per-voxel so the
            // surface block choice (grass/sand/etc) still varies smoothly.
            const float sampleX = quantizeToPlateau(worldX, plateauW);
            const float sampleZ = quantizeToPlateau(worldZ, plateauW);

            const float baseHeight = generateBaseHeight(sampleX, sampleZ, rng);
            auto [humidity, temperature] = m_biomeGen->generateBiomeData(worldX, worldZ, rng);
            const float finalHeight = applyTerrainFeatures(baseHeight, sampleX, sampleZ, rng);

            m_biomeGen->applyEnvironmentalEffects(chunk, x_local, z_local, finalHeight, humidity, temperature);
        }
    }
}

void TerrainGenerator::generateFeatures(Chunk* chunk) {
    if (chunk == nullptr) {
        return;
    }

    const glm::ivec3& chunkPos = chunk->getPosition();
    if (chunkPos.y < 0 || chunkPos.y > 2) {
        return;
    }

    // Initialize random number generator
    int const chunkSeed = m_params.seed + chunkPos.x * 10000 + chunkPos.y * 1000 + chunkPos.z * 100;
    std::mt19937 rng(chunkSeed);
    std::uniform_int_distribution<int> randPos(3, CHUNK_SIZE - 4);

    // Generate each type of feature
    m_featureGen->generateTrees(chunk, rng, randPos);
    m_featureGen->generateRocks(chunk, rng, randPos);
    m_featureGen->generateVegetation(chunk, rng, randPos);
    m_featureGen->generateDecorations(chunk, rng, randPos);
}

float TerrainGenerator::sampleHeight(float voxelX, float voxelZ) const {
    // Quantize XZ to the same plateau grid generateTerrain uses, otherwise
    // VoxelWorld::getHeightAt would return a smooth-noise height that
    // disagrees with the stepped terrain actually placed.
    const float sampleX = quantizeToPlateau(voxelX, m_params.plateauWidthVoxels);
    const float sampleZ = quantizeToPlateau(voxelZ, m_params.plateauWidthVoxels);
    std::mt19937 unused(0); // NOLINT(cert-msc32-c,cert-msc51-cpp) — values discarded; rng is unused inside the helpers.
    const float base = generateBaseHeight(sampleX, sampleZ, unused);
    return applyTerrainFeatures(base, sampleX, sampleZ, unused);
}

} // namespace Sylva
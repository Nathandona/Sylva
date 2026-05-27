#include "terrain_generator.h"
#include "biome_generator.h"
#include "feature_generator.h"
#include "core/logger.h"
#include "world/chunk/chunk.h" // For Chunk, BlockType, BlockData, CHUNK_SIZE
#include "core/config.h" // If params in TerrainGenerator need defaults from config
#include <glm/gtc/noise.hpp>
#include <cmath> // For std::pow, std::abs, sin, etc.
#include <algorithm> // For std::max, std::min

namespace Sylva {

TerrainGenerator::TerrainGenerator(const WorldParams& params)
    : m_params(params),
      m_biomeGen(std::make_unique<BiomeGenerator>(params)),
      m_featureGen(std::make_unique<FeatureGenerator>(params)) {
    Logger::logInfo("TerrainGenerator created.");
}

float TerrainGenerator::generateBaseHeight(float worldX, float worldZ, std::mt19937& rng) const {
    float continentalScale = m_params.noiseScale * 0.2f;
    float continentalNoise = glm::simplex(glm::vec2(worldX * continentalScale, worldZ * continentalScale));
    
    float regionalScale = m_params.noiseScale * 0.5f;
    float regionalNoise = glm::simplex(glm::vec2(worldX * regionalScale, worldZ * regionalScale));
    
    float localScale = m_params.noiseScale;
    float localNoise = glm::simplex(glm::vec2(worldX * localScale, worldZ * localScale));
    
    // Generate detail noise with multiple octaves
    float detailNoiseVal = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float totalAmplitude = 0.0f;
    
    for (int octave = 0; octave < 4; octave++) {
        float octaveScale = m_params.detailScale * frequency;
        detailNoiseVal += amplitude * glm::simplex(glm::vec2(worldX * octaveScale, worldZ * octaveScale));
        totalAmplitude += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    if (totalAmplitude > 0.0f) {
        detailNoiseVal /= totalAmplitude;
    }
    
    // Default weights for noise combination
    float continentalWeight = 0.55f;
    float regionalWeight = 0.30f;
    float localWeight = 0.12f;
    float detailWeight = 0.03f;
    
    // Combine noise values
    float combinedNoise = (continentalNoise * continentalWeight) + 
                         (regionalNoise * regionalWeight) + 
                         (localNoise * localWeight) + 
                         (detailNoiseVal * detailWeight);
    
    // Apply base curve factor
    float curveFactor = sin(worldX * 0.01f) * sin(worldZ * 0.01f) * 0.1f + 0.9f;
    float height = ((combinedNoise + 1.0f) * 0.5f * m_params.maxHeight) * curveFactor;
    
    return height;
}

float TerrainGenerator::applyTerrainFeatures(float baseHeight, float worldX, float worldZ, std::mt19937& rng) const {
    float height = baseHeight;
    
    // Apply additional terrain features
    float featureNoise = glm::simplex(glm::vec2(worldX * 0.05f, worldZ * 0.05f));
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
    int chunkSeed = m_params.seed + chunkPos.x * 10000 + chunkPos.y * 1000 + chunkPos.z * 100;
    std::mt19937 rng(chunkSeed);
    
    for (int z_local = 0; z_local < CHUNK_SIZE; z_local++) {
        for (int x_local = 0; x_local < CHUNK_SIZE; x_local++) {
            float worldX = (chunkPos.x * CHUNK_SIZE) + x_local;
            float worldZ = (chunkPos.z * CHUNK_SIZE) + z_local;
            
            // Generate base terrain height
            float baseHeight = generateBaseHeight(worldX, worldZ, rng);
            
            // Generate biome data
            auto [humidity, temperature] = m_biomeGen->generateBiomeData(worldX, worldZ, rng);
            
            // Apply terrain features
            float finalHeight = applyTerrainFeatures(baseHeight, worldX, worldZ, rng);
            
            // Apply environmental effects (surface blocks, underground layers, water)
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
    int chunkSeed = m_params.seed + chunkPos.x * 10000 + chunkPos.y * 1000 + chunkPos.z * 100;
    std::mt19937 rng(chunkSeed);
    std::uniform_int_distribution<int> randPos(3, CHUNK_SIZE - 4);
    
    // Generate each type of feature
    m_featureGen->generateTrees(chunk, rng, randPos);
    m_featureGen->generateRocks(chunk, rng, randPos);
    m_featureGen->generateVegetation(chunk, rng, randPos);
    m_featureGen->generateDecorations(chunk, rng, randPos);
}

} // namespace Sylva 
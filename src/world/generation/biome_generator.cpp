#include "biome_generator.h"
#include "world/chunk/chunk.h"
#include <glm/gtc/noise.hpp>
#include <cmath>

namespace Sylva {

BiomeGenerator::BiomeGenerator(const WorldParams& params) : m_params(params) {}

std::pair<float, float> BiomeGenerator::generateBiomeData(float worldX, float worldZ, std::mt19937& /*rng*/) const {
    float const humiditySeed = (worldX * 0.01f) + (worldZ * 0.015f) + m_params.seed * 0.001f;
    float humidity = glm::simplex(glm::vec2(worldX * 0.003f, worldZ * 0.003f) + humiditySeed);
    humidity = (humidity + 1.0f) * 0.5f;
    float const temperatureSeed = (worldX * 0.012f) + (worldZ * 0.01f) + m_params.seed * 0.002f;
    float temperature = glm::simplex(glm::vec2(worldX * 0.002f, worldZ * 0.002f) + temperatureSeed);
    temperature = (temperature + 1.0f) * 0.5f;
    return {humidity, temperature};
}

void BiomeGenerator::applyEnvironmentalEffects(Chunk* chunk,
                                               int x_local,
                                               int z_local,
                                               float height,
                                               float humidity,
                                               float temperature) const {
    if (chunk == nullptr)
        return;
    const glm::ivec3& chunkPos = chunk->getPosition();
    float const worldX = (chunkPos.x * CHUNK_SIZE) + x_local;
    float const worldZ = (chunkPos.z * CHUNK_SIZE) + z_local;
    int const terrainHeight = static_cast<int>(height);
    int const localTerrainHeight = terrainHeight - (chunkPos.y * CHUNK_SIZE);
    for (int y_local = 0; y_local < CHUNK_SIZE; y_local++) {
        if (y_local < localTerrainHeight) {
            float const materialNoise = glm::simplex(glm::vec2(worldX * 0.1f, worldZ * 0.1f));
            float const depthNoise = (materialNoise + 1.0f) * 0.5f;
            if (y_local == localTerrainHeight - 1) {
                if (height > m_params.maxHeight * 0.7f && depthNoise > 0.5f) {
                    chunk->setBlock(x_local, y_local, z_local, BlockType::STONE);
                } else if (temperature > 0.7f && humidity < 0.35f) {
                    chunk->setBlock(x_local, y_local, z_local, BlockType::SAND);
                } else if (height < m_params.maxHeight * 0.3f && humidity > 0.4f) {
                    chunk->setBlock(x_local, y_local, z_local, BlockType::SAND);
                } else {
                    chunk->setBlock(x_local, y_local, z_local, BlockType::GRASS);
                }
            } else {
                int const dirtDepth = 2 + static_cast<int>(humidity * 3.0f + depthNoise * 2.0f);
                if (y_local >= localTerrainHeight - dirtDepth) {
                    chunk->setBlock(x_local, y_local, z_local, BlockType::DIRT);
                } else {
                    if (depthNoise > 0.8f && height < m_params.maxHeight * 0.6f) {
                        chunk->setBlock(x_local, y_local, z_local, BlockType::DIRT);
                    } else {
                        chunk->setBlock(x_local, y_local, z_local, BlockType::STONE);
                    }
                }
            }
        } else {
            auto const world_y = static_cast<float>(chunkPos.y * CHUNK_SIZE + y_local);
            float const waterLevel = m_params.maxHeight * 0.25f + m_params.maxHeight * 0.1f * humidity;
            if (world_y < waterLevel && y_local >= localTerrainHeight) {
                chunk->setBlock(x_local, y_local, z_local, BlockType::WATER);
            }
        }
    }
}

} // namespace Sylva
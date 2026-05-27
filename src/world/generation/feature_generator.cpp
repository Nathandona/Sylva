#include "feature_generator.h"
#include "world/chunk/chunk.h"
#include <random>
#include <cmath>
#include <glm/gtc/noise.hpp>

namespace Sylva {

namespace {
// Top non-air, non-water voxel Y in column (x,z). -1 if none.
int findGroundY(Chunk* chunk, int x, int z) {
    for (int y = CHUNK_SIZE - 1; y >= 0; --y) {
        BlockType const b = chunk->getBlock(x, y, z);
        if (b != BlockType::AIR && b != BlockType::WATER)
            return y;
    }
    return -1;
}
} // namespace

FeatureGenerator::FeatureGenerator(const WorldParams& params) : m_params(params) {}

void FeatureGenerator::generateTrees(Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& /*randPos*/) {
    std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    float const treeChance = 0.20f;
    if (rand01(rng) < treeChance) {
        int const numTrees = 1 + static_cast<int>(rand01(rng) * 2.0f);
        for (int i = 0; i < numTrees; i++) {
            int const currentTrunkBaseSideLength = 5 + static_cast<int>(rand01(rng) * 5.0f);
            std::uniform_int_distribution<int> randPosTree(3, CHUNK_SIZE - (3 + currentTrunkBaseSideLength));
            int const treeX = randPosTree(rng);
            int const treeZ = randPosTree(rng);
            int groundY = -1;
            bool canPlaceTrunk = true;
            int minGroundY = CHUNK_SIZE;
            for (int dx = 0; dx < currentTrunkBaseSideLength; ++dx) {
                for (int dz = 0; dz < currentTrunkBaseSideLength; ++dz) {
                    int const currentGroundY = findGroundY(chunk, treeX + dx, treeZ + dz);
                    if (currentGroundY == -1 ||
                        chunk->getBlock(treeX + dx, currentGroundY, treeZ + dz) != BlockType::GRASS) {
                        canPlaceTrunk = false;
                        break;
                    }
                    minGroundY = std::min(minGroundY, currentGroundY);
                }
                if (!canPlaceTrunk)
                    break;
            }
            groundY = minGroundY;
            if (canPlaceTrunk && groundY >= 0) {
                int const trunkHeight = 30 + static_cast<int>(rand01(rng) * 21.0f);
                float const trunkRadius = (currentTrunkBaseSideLength - 1) / 2.0f;
                float const trunkCenterX = (currentTrunkBaseSideLength - 1) / 2.0f;
                float const trunkCenterZ = (currentTrunkBaseSideLength - 1) / 2.0f;
                for (int y_offset = 1; y_offset <= trunkHeight; y_offset++) {
                    if (groundY + y_offset >= CHUNK_SIZE)
                        break;
                    for (int dz_local = 0; dz_local < currentTrunkBaseSideLength; ++dz_local) {
                        for (int dx_local = 0; dx_local < currentTrunkBaseSideLength; ++dx_local) {
                            float const distFromTrunkCenter = std::sqrt(
                                (dx_local - trunkCenterX) * (dx_local - trunkCenterX) +
                                (dz_local - trunkCenterZ) * (dz_local - trunkCenterZ));
                            if (distFromTrunkCenter <= trunkRadius + 0.5f) {
                                if (treeX + dx_local < CHUNK_SIZE && treeZ + dz_local < CHUNK_SIZE) {
                                    chunk->setBlock(treeX + dx_local, groundY + y_offset, treeZ + dz_local, BlockType::WOOD);
                                }
                            }
                        }
                    }
                }
                int const leavesStart = trunkHeight - 8;
                int const leavesEnd = trunkHeight + 20 + static_cast<int>(rand01(rng) * 16.0f);
                bool const isRoundTop = rand01(rng) > 0.4f;
                float const foliageCenterX = treeX + (currentTrunkBaseSideLength - 1) / 2.0f;
                float const foliageCenterZ = treeZ + (currentTrunkBaseSideLength - 1) / 2.0f;
                for (int ly = leavesStart; ly <= leavesEnd; ly++) {
                    if (groundY + ly < 0 || groundY + ly >= CHUNK_SIZE)
                        continue;
                    float heightPosition = static_cast<float>(ly - leavesStart) / (leavesEnd - leavesStart);
                    heightPosition = std::max(0.0f, std::min(1.0f, heightPosition));
                    int baseLeafRadius = isRoundTop ? 18 + static_cast<int>(7.0f * sin(heightPosition * 3.14159f))
                                                    : 25 - static_cast<int>(10.0f * heightPosition);
                    baseLeafRadius = std::max(1, baseLeafRadius);
                    for (int lz_offset = -baseLeafRadius; lz_offset <= baseLeafRadius; lz_offset++) {
                        for (int lx_offset = -baseLeafRadius; lx_offset <= baseLeafRadius; lx_offset++) {
                            int const blockX = static_cast<int>(foliageCenterX + lx_offset);
                            int const blockY = groundY + ly;
                            int const blockZ = static_cast<int>(foliageCenterZ + lz_offset);
                            if (ly < leavesEnd - 3 && blockX >= treeX && blockX < treeX + currentTrunkBaseSideLength &&
                                blockZ >= treeZ && blockZ < treeZ + currentTrunkBaseSideLength) {
                                if (ly < trunkHeight + (currentTrunkBaseSideLength > 3 ? 2 : 1))
                                    continue;
                            }
                            float const dist = std::sqrt(lx_offset * lx_offset + lz_offset * lz_offset);
                            if (dist > baseLeafRadius + (rand01(rng) * 1.0f - 0.5f))
                                continue;
                            if (dist > baseLeafRadius - 1.5f && rand01(rng) < 0.30f)
                                continue;
                            if (blockX < 0 || blockX >= CHUNK_SIZE || blockZ < 0 || blockZ >= CHUNK_SIZE)
                                continue;
                            if (chunk->getBlock(blockX, blockY, blockZ) != BlockType::AIR)
                                continue;
                            chunk->setBlock(blockX, blockY, blockZ, BlockType::LEAVES);
                        }
                    }
                }
            }
        }
    }
}

void FeatureGenerator::generateRocks(Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos) {
    std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    float const rockChance = 0.10f;
    if (rand01(rng) < rockChance) {
        int const numRocks = 1 + static_cast<int>(rand01(rng) * 3.0f);
        for (int i = 0; i < numRocks; i++) {
            int const rockX = randPos(rng);
            int const rockZ = randPos(rng);
            int const groundY = findGroundY(chunk, rockX, rockZ);
            if (groundY >= 0) {
                bool const isFlat = rand01(rng) > 0.7f;
                int const rockHeight = isFlat ? 1 : 1 + static_cast<int>(rand01(rng) * 2.0f);
                int const rockRadius = 1 + static_cast<int>(rand01(rng) * 2.2f);
                for (int ry = 0; ry < rockHeight; ry++) {
                    float const heightFactor = isFlat ? 0.3f : (static_cast<float>(ry) / rockHeight);
                    int const layerRadius = std::max(1, static_cast<int>(rockRadius * (1.0f - heightFactor * 0.7f)));
                    for (int rz_local = -layerRadius; rz_local <= layerRadius; rz_local++) {
                        for (int rx_local = -layerRadius; rx_local <= layerRadius; rx_local++) {
                            float const distortion = (rx_local * 74.21f + rz_local * 45.13f) * 0.01f;
                            float const dist = std::sqrt(rx_local * rx_local + rz_local * rz_local) +
                                               (distortion - floor(distortion));
                            if (dist > layerRadius * (1.0f + (rand01(rng) * 0.2f - 0.1f)))
                                continue;
                            int const blockX = rockX + rx_local;
                            int const blockY = groundY + ry + 1;
                            int const blockZ = rockZ + rz_local;
                            if (blockX < 0 || blockX >= CHUNK_SIZE || blockY < 0 || blockY >= CHUNK_SIZE ||
                                blockZ < 0 || blockZ >= CHUNK_SIZE)
                                continue;
                            if (chunk->getBlock(blockX, blockY, blockZ) != BlockType::AIR)
                                continue;
                            chunk->setBlock(blockX, blockY, blockZ, BlockType::STONE);
                        }
                    }
                }
            }
        }
    }
}

void FeatureGenerator::generateVegetation(Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos) {
    std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    float const flowerChance = 0.30f;
    if (rand01(rng) < flowerChance) {
        int const numFlowerPatches = 1 + static_cast<int>(rand01(rng) * 3.0f);
        for (int i = 0; i < numFlowerPatches; i++) {
            int const patchX = randPos(rng);
            int const patchZ = randPos(rng);
            int const patchRadius = 1 + static_cast<int>(rand01(rng) * 3.0f);
            int const numFlowers = 3 + static_cast<int>(rand01(rng) * patchRadius * 3.0f);
            for (int f = 0; f < numFlowers; f++) {
                float const angle = rand01(rng) * 6.28318f;
                float const distance = rand01(rng) * patchRadius;
                int flowerX = patchX + static_cast<int>(cos(angle) * distance);
                int flowerZ = patchZ + static_cast<int>(sin(angle) * distance);
                flowerX = std::max(0, std::min(flowerX, CHUNK_SIZE - 1));
                flowerZ = std::max(0, std::min(flowerZ, CHUNK_SIZE - 1));
                int const groundY = findGroundY(chunk, flowerX, flowerZ);
                if (groundY >= 0 && chunk->getBlock(flowerX, groundY, flowerZ) == BlockType::GRASS) {
                    chunk->setBlock(flowerX, groundY + 1, flowerZ, BlockType::LEAVES);
                }
            }
        }
    }
}

void FeatureGenerator::generateDecorations(Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos) {
    std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    float const mushroomChance = 0.15f;
    float const bushChance = 0.20f;
    if (rand01(rng) < mushroomChance) {
        int const numMushrooms = 2 + static_cast<int>(rand01(rng) * 4.0f);
        for (int i = 0; i < numMushrooms; i++) {
            int const mushroomX = randPos(rng);
            int const mushroomZ = randPos(rng);
            int const groundY = findGroundY(chunk, mushroomX, mushroomZ);
            if (groundY >= 0 && (chunk->getBlock(mushroomX, groundY, mushroomZ) == BlockType::DIRT ||
                                 chunk->getBlock(mushroomX, groundY, mushroomZ) == BlockType::GRASS)) {
                chunk->setBlock(mushroomX, groundY + 1, mushroomZ, BlockType::WOOD);
                if (groundY + 2 < CHUNK_SIZE) {
                    chunk->setBlock(mushroomX, groundY + 2, mushroomZ, BlockType::LEAVES);
                    int const capExtends[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                    for (const auto& capExtend : capExtends) {
                        int const capX = mushroomX + capExtend[0];
                        int const capZ = mushroomZ + capExtend[1];
                        if (capX >= 0 && capX < CHUNK_SIZE && capZ >= 0 && capZ < CHUNK_SIZE) {
                            if (chunk->getBlock(capX, groundY + 2, capZ) == BlockType::AIR) {
                                chunk->setBlock(capX, groundY + 2, capZ, BlockType::LEAVES);
                            }
                        }
                    }
                }
            }
        }
    }
    if (rand01(rng) < bushChance) {
        int const numBushes = 2 + static_cast<int>(rand01(rng) * 3.0f);
        for (int i = 0; i < numBushes; i++) {
            int const bushX = randPos(rng);
            int const bushZ = randPos(rng);
            int const groundY = findGroundY(chunk, bushX, bushZ);
            if (groundY >= 0 && chunk->getBlock(bushX, groundY, bushZ) == BlockType::GRASS) {
                int const bushHeight = 1 + static_cast<int>(rand01(rng) * 2.0f);
                for (int by = 0; by < bushHeight; by++) {
                    if (groundY + by + 1 >= CHUNK_SIZE)
                        continue;
                    int bushRadius = 1;
                    if (by == 0 && bushHeight > 1)
                        bushRadius = 2;
                    for (int bz_local = -bushRadius; bz_local <= bushRadius; bz_local++) {
                        for (int bx_local = -bushRadius; bx_local <= bushRadius; bx_local++) {
                            float const dist = std::sqrt(bx_local * bx_local + bz_local * bz_local);
                            if (dist > bushRadius)
                                continue;
                            if (dist > bushRadius - 0.5f && rand01(rng) < 0.4f)
                                continue;
                            int const blockX = bushX + bx_local;
                            int const blockY = groundY + by + 1;
                            int const blockZ = bushZ + bz_local;
                            if (blockX < 0 || blockX >= CHUNK_SIZE || blockY < 0 || blockY >= CHUNK_SIZE ||
                                blockZ < 0 || blockZ >= CHUNK_SIZE)
                                continue;
                            if (chunk->getBlock(blockX, blockY, blockZ) != BlockType::AIR)
                                continue;
                            chunk->setBlock(blockX, blockY, blockZ, BlockType::LEAVES);
                        }
                    }
                }
            }
        }
    }
}

} // namespace Sylva
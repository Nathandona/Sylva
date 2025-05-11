#include "feature_generator.h"
#include "world/chunk/chunk.h"
#include <random>
#include <cmath>
#include <glm/gtc/noise.hpp>

namespace Sylva {

FeatureGenerator::FeatureGenerator(const WorldParams& params)
    : m_params(params) {}

void FeatureGenerator::generateTrees(Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos) {
    std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    float treeChance = 0.20f;
    if (rand01(rng) < treeChance) {
        int numTrees = 1 + static_cast<int>(rand01(rng) * 2.0f);
        for (int i = 0; i < numTrees; i++) {
            int currentTrunkBaseSideLength = 5 + static_cast<int>(rand01(rng) * 5.0f);
            std::uniform_int_distribution<int> randPosTree(3, CHUNK_SIZE - (3 + currentTrunkBaseSideLength));
            int treeX = randPosTree(rng);
            int treeZ = randPosTree(rng);
            int groundY = -1;
            bool canPlaceTrunk = true;
            int minGroundY = CHUNK_SIZE;
            for (int dx = 0; dx < currentTrunkBaseSideLength; ++dx) {
                for (int dz = 0; dz < currentTrunkBaseSideLength; ++dz) {
                    int currentGroundY = -1;
                    for (int y_local = CHUNK_SIZE - 1; y_local >= 0; y_local--) {
                        BlockType block = chunk->getBlock(treeX + dx, y_local, treeZ + dz);
                        if (block != BlockType::AIR && block != BlockType::WATER) {
                            currentGroundY = y_local;
                            break;
                        }
                    }
                    if (currentGroundY == -1 || chunk->getBlock(treeX + dx, currentGroundY, treeZ + dz) != BlockType::GRASS) {
                        canPlaceTrunk = false;
                        break;
                    }
                    minGroundY = std::min(minGroundY, currentGroundY);
                }
                if (!canPlaceTrunk) break;
            }
            groundY = minGroundY;
            if (canPlaceTrunk && groundY >= 0) {
                int trunkHeight = 30 + static_cast<int>(rand01(rng) * 21.0f);
                float trunkRadius = (currentTrunkBaseSideLength - 1) / 2.0f;
                float trunkCenterX = (currentTrunkBaseSideLength - 1) / 2.0f;
                float trunkCenterZ = (currentTrunkBaseSideLength - 1) / 2.0f;
                for (int y_offset = 1; y_offset <= trunkHeight; y_offset++) {
                    if (groundY + y_offset >= CHUNK_SIZE) break;
                    for (int dz_local = 0; dz_local < currentTrunkBaseSideLength; ++dz_local) {
                        for (int dx_local = 0; dx_local < currentTrunkBaseSideLength; ++dx_local) {
                            float distFromTrunkCenter = std::sqrt(
                                (dx_local - trunkCenterX) * (dx_local - trunkCenterX) +
                                (dz_local - trunkCenterZ) * (dz_local - trunkCenterZ)
                            );
                            if (distFromTrunkCenter <= trunkRadius + 0.5f) {
                                if (treeX + dx_local < CHUNK_SIZE && treeZ + dz_local < CHUNK_SIZE) {
                                    chunk->setBlock(treeX + dx_local, groundY + y_offset, treeZ + dz_local, BlockType::WOOD);
                                }
                            }
                        }
                    }
                }
                int leavesStart = trunkHeight - 8;
                int leavesEnd = trunkHeight + 20 + static_cast<int>(rand01(rng) * 16.0f);
                bool isRoundTop = rand01(rng) > 0.4f;
                float foliageCenterX = treeX + (currentTrunkBaseSideLength - 1) / 2.0f;
                float foliageCenterZ = treeZ + (currentTrunkBaseSideLength - 1) / 2.0f;
                for (int ly = leavesStart; ly <= leavesEnd; ly++) {
                    if (groundY + ly < 0 || groundY + ly >= CHUNK_SIZE) continue;
                    float heightPosition = static_cast<float>(ly - leavesStart) / (leavesEnd - leavesStart);
                    heightPosition = std::max(0.0f, std::min(1.0f, heightPosition));
                    int baseLeafRadius = isRoundTop ? 
                        18 + static_cast<int>(7.0f * sin(heightPosition * 3.14159f)) :
                        25 - static_cast<int>(10.0f * heightPosition);
                    baseLeafRadius = std::max(1, baseLeafRadius);
                    for (int lz_offset = -baseLeafRadius; lz_offset <= baseLeafRadius; lz_offset++) {
                        for (int lx_offset = -baseLeafRadius; lx_offset <= baseLeafRadius; lx_offset++) {
                            int blockX = static_cast<int>(foliageCenterX + lx_offset);
                            int blockY = groundY + ly;
                            int blockZ = static_cast<int>(foliageCenterZ + lz_offset);
                            if (ly < leavesEnd - 3 &&
                                blockX >= treeX && blockX < treeX + currentTrunkBaseSideLength &&
                                blockZ >= treeZ && blockZ < treeZ + currentTrunkBaseSideLength) {
                                if (ly < trunkHeight + (currentTrunkBaseSideLength > 3 ? 2 : 1)) continue;
                            }
                            float dist = std::sqrt(lx_offset*lx_offset + lz_offset*lz_offset);
                            if (dist > baseLeafRadius + (rand01(rng) * 1.0f - 0.5f)) continue;
                            if (dist > baseLeafRadius - 1.5f && rand01(rng) < 0.30f) continue;
                            if (blockX < 0 || blockX >= CHUNK_SIZE || blockZ < 0 || blockZ >= CHUNK_SIZE) continue;
                            if (chunk->getBlock(blockX, blockY, blockZ) != BlockType::AIR) continue;
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
    float rockChance = 0.10f;
    if (rand01(rng) < rockChance) {
        int numRocks = 1 + static_cast<int>(rand01(rng) * 3.0f);
        for (int i = 0; i < numRocks; i++) {
            int rockX = randPos(rng);
            int rockZ = randPos(rng);
            int groundY = -1;
            for (int y_local = CHUNK_SIZE - 1; y_local >= 0; y_local--) {
                BlockType block = chunk->getBlock(rockX, y_local, rockZ);
                if (block != BlockType::AIR && block != BlockType::WATER) {
                    groundY = y_local;
                    break;
                }
            }
            if (groundY >= 0) {
                bool isFlat = rand01(rng) > 0.7f;
                int rockHeight = isFlat ? 1 : 1 + static_cast<int>(rand01(rng) * 2.0f);
                int rockRadius = 1 + static_cast<int>(rand01(rng) * 2.2f);
                for (int ry = 0; ry < rockHeight; ry++) {
                    float heightFactor = isFlat ? 0.3f : (static_cast<float>(ry) / rockHeight);
                    int layerRadius = std::max(1, static_cast<int>(rockRadius * (1.0f - heightFactor * 0.7f)));
                    for (int rz_local = -layerRadius; rz_local <= layerRadius; rz_local++) {
                        for (int rx_local = -layerRadius; rx_local <= layerRadius; rx_local++) {
                            float distortion = (rx_local * 74.21f + rz_local * 45.13f) * 0.01f;
                            float dist = std::sqrt(rx_local*rx_local + rz_local*rz_local) + (distortion - floor(distortion));
                            if (dist > layerRadius * (1.0f + (rand01(rng) * 0.2f - 0.1f))) continue;
                            int blockX = rockX + rx_local;
                            int blockY = groundY + ry + 1;
                            int blockZ = rockZ + rz_local;
                            if (blockX < 0 || blockX >= CHUNK_SIZE || blockY < 0 || blockY >= CHUNK_SIZE || 
                                blockZ < 0 || blockZ >= CHUNK_SIZE) continue;
                            if (chunk->getBlock(blockX, blockY, blockZ) != BlockType::AIR) continue;
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
    float flowerChance = 0.30f;
    if (rand01(rng) < flowerChance) {
        int numFlowerPatches = 1 + static_cast<int>(rand01(rng) * 3.0f);
        for (int i = 0; i < numFlowerPatches; i++) {
            int patchX = randPos(rng);
            int patchZ = randPos(rng);
            int patchRadius = 1 + static_cast<int>(rand01(rng) * 3.0f);
            int numFlowers = 3 + static_cast<int>(rand01(rng) * patchRadius * 3.0f);
            for (int f = 0; f < numFlowers; f++) {
                float angle = rand01(rng) * 6.28318f;
                float distance = rand01(rng) * patchRadius;
                int flowerX = patchX + static_cast<int>(cos(angle) * distance);
                int flowerZ = patchZ + static_cast<int>(sin(angle) * distance);
                flowerX = std::max(0, std::min(flowerX, CHUNK_SIZE - 1));
                flowerZ = std::max(0, std::min(flowerZ, CHUNK_SIZE - 1));
                int groundY = -1;
                for (int y_local = CHUNK_SIZE - 1; y_local >= 0; y_local--) {
                    BlockType block = chunk->getBlock(flowerX, y_local, flowerZ);
                    if (block != BlockType::AIR && block != BlockType::WATER) {
                        groundY = y_local;
                        break;
                    }
                }
                if (groundY >= 0 && chunk->getBlock(flowerX, groundY, flowerZ) == BlockType::GRASS) {
                    chunk->setBlock(flowerX, groundY + 1, flowerZ, BlockType::LEAVES);
                }
            }
        }
    }
}

void FeatureGenerator::generateDecorations(Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos) {
    std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    float mushroomChance = 0.15f;
    float bushChance = 0.20f;
    if (rand01(rng) < mushroomChance) {
        int numMushrooms = 2 + static_cast<int>(rand01(rng) * 4.0f);
        for (int i = 0; i < numMushrooms; i++) {
            int mushroomX = randPos(rng);
            int mushroomZ = randPos(rng);
            int groundY = -1;
            for (int y_local = CHUNK_SIZE - 1; y_local >= 0; y_local--) {
                BlockType block = chunk->getBlock(mushroomX, y_local, mushroomZ);
                if (block != BlockType::AIR && block != BlockType::WATER) {
                    groundY = y_local;
                    break;
                }
            }
            if (groundY >= 0 && (chunk->getBlock(mushroomX, groundY, mushroomZ) == BlockType::DIRT || 
                                chunk->getBlock(mushroomX, groundY, mushroomZ) == BlockType::GRASS)) {
                chunk->setBlock(mushroomX, groundY + 1, mushroomZ, BlockType::WOOD);
                if (groundY + 2 < CHUNK_SIZE) {
                    chunk->setBlock(mushroomX, groundY + 2, mushroomZ, BlockType::LEAVES);
                    int capExtends[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
                    for (int c = 0; c < 4; c++) {
                        int capX = mushroomX + capExtends[c][0];
                        int capZ = mushroomZ + capExtends[c][1];
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
        int numBushes = 2 + static_cast<int>(rand01(rng) * 3.0f);
        for (int i = 0; i < numBushes; i++) {
            int bushX = randPos(rng);
            int bushZ = randPos(rng);
            int groundY = -1;
            for (int y_local = CHUNK_SIZE - 1; y_local >= 0; y_local--) {
                BlockType block = chunk->getBlock(bushX, y_local, bushZ);
                if (block != BlockType::AIR && block != BlockType::WATER) {
                    groundY = y_local;
                    break;
                }
            }
            if (groundY >= 0 && chunk->getBlock(bushX, groundY, bushZ) == BlockType::GRASS) {
                int bushHeight = 1 + static_cast<int>(rand01(rng) * 2.0f);
                for (int by = 0; by < bushHeight; by++) {
                    if (groundY + by + 1 >= CHUNK_SIZE) continue;
                    int bushRadius = 1;
                    if (by == 0 && bushHeight > 1) bushRadius = 2;
                    for (int bz_local = -bushRadius; bz_local <= bushRadius; bz_local++) {
                        for (int bx_local = -bushRadius; bx_local <= bushRadius; bx_local++) {
                            float dist = std::sqrt(bx_local*bx_local + bz_local*bz_local);
                            if (dist > bushRadius) continue;
                            if (dist > bushRadius - 0.5f && rand01(rng) < 0.4f) continue;
                            int blockX = bushX + bx_local;
                            int blockY = groundY + by + 1;
                            int blockZ = bushZ + bz_local;
                            if (blockX < 0 || blockX >= CHUNK_SIZE || blockY < 0 || blockY >= CHUNK_SIZE || 
                                blockZ < 0 || blockZ >= CHUNK_SIZE) continue;
                            if (chunk->getBlock(blockX, blockY, blockZ) != BlockType::AIR) continue;
                            chunk->setBlock(blockX, blockY, blockZ, BlockType::LEAVES);
                        }
                    }
                }
            }
        }
    }
}

} // namespace Sylva 
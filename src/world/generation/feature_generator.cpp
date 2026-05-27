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
    // Tree silhouette tuned for micro-voxel scale (cellSize=0.25, player 1.8m):
    //   trunk radius 1 voxel  → ~0.5m thick
    //   trunk height ~16 vox  → ~4m tall (~2.2x player)
    //   crown radius  ~6 vox  → ~1.5m wide canopy
    // Previously: 5-10 voxel trunk × 30-51 voxel height with 18-25 voxel leaf
    // radius → 9-12m wide leaf clouds dominated the world.
    constexpr int kTrunkRadius = 1;
    constexpr int kTrunkHeightBase = 16;
    constexpr int kTrunkHeightJitter = 6; // ±3 around base
    constexpr int kCrownRadiusBase = 6;
    constexpr int kCrownRadiusJitter = 4; // ±2

    std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    // ~40% of chunks contain trees; 1-2 trees each.
    if (rand01(rng) > 0.40f) {
        return;
    }
    const int numTrees = 1 + static_cast<int>(rand01(rng) * 2.0f);

    // Keep crown fully inside the chunk to avoid clipped foliage at borders.
    const int margin = kCrownRadiusBase + kCrownRadiusJitter / 2 + 1;
    if (CHUNK_SIZE - 2 * margin < 1) {
        return; // chunk too small for the configured crown
    }
    std::uniform_int_distribution<int> spawnXZ(margin, CHUNK_SIZE - margin - 1);

    for (int t = 0; t < numTrees; ++t) {
        const int cx = spawnXZ(rng);
        const int cz = spawnXZ(rng);

        const int groundY = findGroundY(chunk, cx, cz);
        if (groundY < 0) {
            continue; // no terrain here
        }
        if (chunk->getBlock(cx, groundY, cz) != BlockType::GRASS) {
            continue; // only grow on grass
        }

        const int trunkHeight = kTrunkHeightBase + static_cast<int>(rand01(rng) * kTrunkHeightJitter) -
                                kTrunkHeightJitter / 2;
        const int crownRadius =
            std::max(2, kCrownRadiusBase + static_cast<int>(rand01(rng) * kCrownRadiusJitter) - kCrownRadiusJitter / 2);
        const int crownRSq = crownRadius * crownRadius;
        const int trunkRSq = kTrunkRadius * kTrunkRadius;

        // Trunk: vertical cylinder of WOOD from just above ground up trunkHeight voxels.
        const int trunkTopY = std::min(groundY + trunkHeight, CHUNK_SIZE - 1);
        for (int y = groundY + 1; y <= trunkTopY; ++y) {
            for (int dz = -kTrunkRadius; dz <= kTrunkRadius; ++dz) {
                for (int dx = -kTrunkRadius; dx <= kTrunkRadius; ++dx) {
                    if (dx * dx + dz * dz > trunkRSq) {
                        continue;
                    }
                    const int x = cx + dx;
                    const int z = cz + dz;
                    if (x < 0 || x >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
                        continue;
                    }
                    chunk->setBlock(x, y, z, BlockType::WOOD);
                }
            }
        }

        // Crown: sphere of LEAVES centered at the trunk top, with light edge
        // noise so the silhouette doesn't read as a perfect ball.
        const int crownCenterY = trunkTopY;
        for (int dy = -crownRadius; dy <= crownRadius; ++dy) {
            for (int dz = -crownRadius; dz <= crownRadius; ++dz) {
                for (int dx = -crownRadius; dx <= crownRadius; ++dx) {
                    const int dSq = dx * dx + dy * dy + dz * dz;
                    if (dSq > crownRSq) {
                        continue;
                    }
                    // Punch ~30% of voxels near the outer shell to break the
                    // smooth ball into something foliage-shaped.
                    if (dSq > crownRSq - crownRadius * 2 && rand01(rng) < 0.30f) {
                        continue;
                    }
                    const int x = cx + dx;
                    const int y = crownCenterY + dy;
                    const int z = cz + dz;
                    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
                        continue;
                    }
                    if (chunk->getBlock(x, y, z) != BlockType::AIR) {
                        continue; // don't overwrite trunk or other blocks
                    }
                    chunk->setBlock(x, y, z, BlockType::LEAVES);
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
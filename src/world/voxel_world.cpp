#include "voxel_world.h"
#include "core/config.h"
#include "core/logger.h"
#include "renderer/camera.h"
#include "renderer/shader.h"
#include "player.h"
#include <glad/glad.h>
#include <glm/gtc/noise.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace Sylva {

VoxelWorld::VoxelWorld() 
    : m_shader(nullptr) {
    // Load world parameters from config if available
    m_params.terrainSize = Config::getInt("World.terrain_size", m_params.terrainSize);
    m_params.cellSize = Config::getFloat("World.cell_size", m_params.cellSize);
    m_params.maxHeight = Config::getFloat("World.max_height", m_params.maxHeight);
    m_params.seed = Config::getInt("World.seed", m_params.seed);
    m_worldSizeInChunks = Config::getInt("World.size_in_chunks", m_worldSizeInChunks);
    m_viewDistanceInChunks = Config::getInt("World.view_distance", m_viewDistanceInChunks);
    
    // Additional micro-voxel specific parameters
    m_params.cellSize = Config::getFloat("World.cell_size", 0.25f);  // Smaller block size for micro-voxels
    m_params.noiseScale = Config::getFloat("World.noise_scale", 0.05f);  // Adjusted default: 0.02f * 2.5
    m_params.detailScale = Config::getFloat("World.detail_scale", 0.20f); // Adjusted default: 0.08f * 2.5
    m_params.terrainSmoothness = Config::getFloat("World.terrain_smoothness", 0.6f); // Higher values = smoother terrain
    
    Logger::logInfo("Micro-voxel world created with size " + std::to_string(m_worldSizeInChunks) + 
                   " chunks and view distance " + std::to_string(m_viewDistanceInChunks) + " chunks");
}

VoxelWorld::VoxelWorld(const WorldParams& params) 
    : m_params(params),
      m_shader(nullptr) {
    m_worldSizeInChunks = Config::getInt("World.size_in_chunks", m_worldSizeInChunks);
    m_viewDistanceInChunks = Config::getInt("World.view_distance", m_viewDistanceInChunks);
    
    Logger::logInfo("VoxelWorld created with custom parameters");
}

VoxelWorld::~VoxelWorld() {
    if (m_shader != nullptr) {
        delete m_shader;
        m_shader = nullptr;
    }
    
    // Clean up debug resources
    if (m_debugVAO != 0) {
        glDeleteVertexArrays(1, &m_debugVAO);
        m_debugVAO = 0;
    }
    
    if (m_debugVBO != 0) {
        glDeleteBuffers(1, &m_debugVBO);
        m_debugVBO = 0;
    }
    
    m_chunks.clear();
}

bool VoxelWorld::initializeGraphics() {
    Logger::logInfo("Initializing micro-voxel world graphics");
    
    // Initialize block data
    BlockData::initialize();
    
    // Create shader
    return createShader();
}

bool VoxelWorld::createShader() {
    // Clean up old shader if it exists
    if (m_shader != nullptr) {
        delete m_shader;
        m_shader = nullptr;
    }
    
    // Create new shader
    try {
        m_shader = new Shader("assets/shaders/voxel.vert", "assets/shaders/voxel.frag");
        Logger::logInfo("Voxel shader created successfully");
        return true;
    } catch (const std::exception& e) {
        Logger::logError("Failed to create voxel shader: " + std::string(e.what()));
        return false;
    }
}

void VoxelWorld::generateWorld(const glm::vec3& playerPosition) {
    Logger::logInfo("Generating micro-voxel world around player position (" + 
                   std::to_string(playerPosition.x) + ", " +
                   std::to_string(playerPosition.y) + ", " +
                   std::to_string(playerPosition.z) + ")");
    
    // Convert player position to chunk coordinates
    glm::ivec3 centerChunkPos = Chunk::worldToChunkPos(playerPosition);
    
    // Generate chunks in a square around the player with greater height range
    for (int y = -1; y <= 7; y++) {  // Expanded vertical range to Y=7
        for (int z = -m_worldSizeInChunks; z <= m_worldSizeInChunks; z++) {
            for (int x = -m_worldSizeInChunks; x <= m_worldSizeInChunks; x++) {
                glm::ivec3 chunkPos = centerChunkPos + glm::ivec3(x, y, z);
                
                // Create chunk if it doesn't exist
                Chunk* chunk = getChunk(chunkPos);
                if (chunk == nullptr) {
                    chunk = createChunk(chunkPos);
                    
                    // Generate terrain for the chunk with micro-voxels
                    generateTerrain(chunk);
                    
                    // Add detailed features
                    generateFeatures(chunk);
                }
            }
        }
    }
    
    // Update chunk meshes
    updateChunkMeshes(centerChunkPos);
    
    Logger::logInfo("Micro-voxel world generation complete");
}

void VoxelWorld::update(float deltaTime, const glm::vec3& playerPosition) {
    // Convert player position to chunk coordinates
    glm::ivec3 playerChunkPos = Chunk::worldToChunkPos(playerPosition);
    
    // Check if we need to generate new chunks or update meshes
    static glm::ivec3 lastPlayerChunkPos(-999999, -999999, -999999);
    
    if (playerChunkPos != lastPlayerChunkPos) {
        // Player moved to a new chunk, update visible chunks
        
        // Generate new chunks in view distance
        for (int y = -1; y <= 7; y++) { // Expanded vertical range to Y=7
            for (int z = -m_viewDistanceInChunks; z <= m_viewDistanceInChunks; z++) {
                for (int x = -m_viewDistanceInChunks; x <= m_viewDistanceInChunks; x++) {
                    glm::ivec3 chunkPos = playerChunkPos + glm::ivec3(x, y, z);
                    
                    // Check if this chunk is within view distance
                    float distance = sqrt(x*x + z*z);
                    if (distance <= m_viewDistanceInChunks) {
                        // Create chunk if it doesn't exist
                        Chunk* chunk = getChunk(chunkPos);
                        if (chunk == nullptr) {
                            chunk = createChunk(chunkPos);
                            generateTerrain(chunk);
                            generateFeatures(chunk);
                        }
                    }
                }
            }
        }
        
        // Update chunk meshes
        updateChunkMeshes(playerChunkPos);
        
        // TODO: Unload chunks that are too far away
        
        lastPlayerChunkPos = playerChunkPos;
    }
}

void VoxelWorld::render(const Camera& camera) {
    if (m_shader == nullptr) {
        Logger::logWarning("Cannot render voxel world, shader not initialized");
        return;
    }
    
    // Get camera matrices
    glm::mat4 viewMatrix = camera.getViewMatrix();
    glm::mat4 projectionMatrix = camera.getProjectionMatrix(16.0f / 9.0f); // TODO: Get actual aspect ratio
    
    // Set up directional light for "Pelican Harbor" stylized aesthetic
    // Warm, slightly angled sunlight creates soft shadows and highlights
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    
    // Activate shader before setting uniforms
    m_shader->use();
    
    // Pass light direction to shader
    m_shader->setVec3("lightDir", lightDir);

    // Get lighting colors from config or use defaults
    glm::vec3 lightColor(
        Config::getFloat("Lighting.light_color.x", 1.0f),
        Config::getFloat("Lighting.light_color.y", 0.95f),
        Config::getFloat("Lighting.light_color.z", 0.8f)
    ); // Warm white
    glm::vec3 ambientColor(
        Config::getFloat("Lighting.ambient_color.x", 0.45f), // Slightly warmer and less blue
        Config::getFloat("Lighting.ambient_color.y", 0.45f),
        Config::getFloat("Lighting.ambient_color.z", 0.45f) 
    ); // Neutral warmish ambient, slightly reduced from a bluish 0.5 on Z

    m_shader->setVec3("uniformLightColor", lightColor);
    m_shader->setVec3("uniformAmbientColor", ambientColor);
    
    // Render each chunk
    for (const auto& pair : m_chunks) {
        pair.second->render(m_shader, viewMatrix, projectionMatrix);
    }
    
    Logger::logDebug("Voxel world rendered with " + std::to_string(m_chunks.size()) + " chunks");
}

Chunk* VoxelWorld::getChunk(const glm::ivec3& chunkPos) const {
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

Chunk* VoxelWorld::createChunk(const glm::ivec3& chunkPos) {
    // Create new chunk
    auto chunk = std::make_unique<Chunk>(chunkPos);
    
    // Store in map and return raw pointer
    Chunk* chunkPtr = chunk.get();
    m_chunks[chunkPos] = std::move(chunk);
    
    // When a new chunk is created, its direct neighbors might need their meshes updated
    // because they now have a new adjacent chunk where there was none (or assumed air).
    const glm::ivec3 neighborOffsets[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };
    for (const auto& offset : neighborOffsets) {
        Chunk* neighbor = getChunk(chunkPos + offset);
        if (neighbor) {
            neighbor->markModified();
        }
    }
    
    return chunkPtr;
}

void VoxelWorld::generateTerrain(Chunk* chunk) {
    if (chunk == nullptr) {
        return;
    }
    
    // Get chunk position
    const glm::ivec3& chunkPos = chunk->getPosition();
    
    // Seed for this chunk
    int chunkSeed = m_params.seed + 
                    chunkPos.x * 10000 + 
                    chunkPos.y * 1000 + 
                    chunkPos.z * 100;
    
    std::mt19937 rng(chunkSeed);
    std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    
    // Generate a heightmap for this chunk with multi-octave noise for detailed, organic terrain
    // For each column in the chunk
    for (int z = 0; z < CHUNK_SIZE; z++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            // Calculate world position for this column
            float worldX = (chunkPos.x * CHUNK_SIZE) + x;
            float worldZ = (chunkPos.z * CHUNK_SIZE) + z;
            
            // ==== Multi-layered terrain generation with variable parameters ====
            
            // Base continental shape - very large scale features (hills, valleys)
            float continentalScale = m_params.noiseScale * 0.2f;
            float continentalNoise = glm::simplex(glm::vec2(worldX * continentalScale, worldZ * continentalScale));
            
            // Regional features - medium scale terrain (small hills, depressions)
            float regionalScale = m_params.noiseScale * 0.5f;
            float regionalNoise = glm::simplex(glm::vec2(worldX * regionalScale, worldZ * regionalScale));
            
            // Local terrain - small scale details
            float localScale = m_params.noiseScale;
            float localNoise = glm::simplex(glm::vec2(worldX * localScale, worldZ * localScale));
            
            // Add micro-detail - very small scale features for that handcrafted look
            float detailNoise = 0.0f;
            float amplitude = 1.0f;
            float frequency = 1.0f;
            float totalAmplitude = 0.0f;
            
            // Add multiple octaves of noise for micro-detail
            for (int octave = 0; octave < 4; octave++) {
                float octaveScale = m_params.detailScale * frequency;
                detailNoise += amplitude * glm::simplex(glm::vec2(worldX * octaveScale, worldZ * octaveScale));
                totalAmplitude += amplitude;
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }
            
            // Normalize the detail noise
            if (totalAmplitude > 0.0f) { // Avoid division by zero if amplitude starts at 0
                detailNoise /= totalAmplitude;
            }
            
            // Calculate biome influence based on position and noise
            float humiditySeed = (worldX * 0.01f) + (worldZ * 0.015f) + m_params.seed * 0.001f;
            float humidity = glm::simplex(glm::vec2(worldX * 0.003f, worldZ * 0.003f) + humiditySeed);
            humidity = (humidity + 1.0f) * 0.5f; // Normalize to 0-1
            
            float temperatureSeed = (worldX * 0.012f) + (worldZ * 0.01f) + m_params.seed * 0.002f;
            float temperature = glm::simplex(glm::vec2(worldX * 0.002f, worldZ * 0.002f) + temperatureSeed);
            temperature = (temperature + 1.0f) * 0.5f; // Normalize to 0-1
            
            // Weight the terrain layers differently based on "biome" characteristics
            float continentalWeight = 0.55f; // Adjusted
            float regionalWeight = 0.30f;    // Adjusted
            float localWeight = 0.12f;       // Adjusted
            float detailWeight = 0.03f;      // Adjusted
            
            // In more humid areas, increase regional variation (more rolling hills)
            if (humidity > 0.6f) {
                regionalWeight = 0.35f;
                continentalWeight = 0.5f;
            }
            
            // In warmer areas, smoother terrain
            if (temperature > 0.7f) {
                detailWeight = 0.03f;
                localWeight = 0.07f;
            }
            
            // Combine the different terrain layers with their weights
            float combinedNoise = (continentalNoise * continentalWeight) + 
                                 (regionalNoise * regionalWeight) + 
                                 (localNoise * localWeight) + 
                                 (detailNoise * detailWeight);
            
            // Apply gentle curves for more natural landforms
            float curveFactor = sin(worldX * 0.01f) * sin(worldZ * 0.01f) * 0.1f + 0.9f;
            
            // Normalize to [0, 1] and scale to desired height
            float height = ((combinedNoise + 1.0f) * 0.5f * m_params.maxHeight) * curveFactor;
            
            // Add sporadic terrain features
            float featureNoise = glm::simplex(glm::vec2(worldX * 0.05f, worldZ * 0.05f));
            if (featureNoise > 0.7f) {
                // Create occasional higher areas (small hills or rock formations)
                height += featureNoise * 3.0f;
            } else if (featureNoise < -0.7f) {
                // Create occasional depressions
                height -= fabsf(featureNoise) * 2.0f;
            }
            
            // Ensure minimum height
            height = std::max(height, 5.0f);
            
            // Round to integer height
            int terrainHeight = static_cast<int>(height);
            
            // Get chunk-local height (offset by chunk Y position)
            int localTerrainHeight = terrainHeight - (chunkPos.y * CHUNK_SIZE);
            
            // Fill blocks below terrain height with varied materials
            for (int y = 0; y < CHUNK_SIZE; y++) {
                if (y < localTerrainHeight) {
                    // Determine block type based on depth, noise, and biome characteristics
                    // Generate varied noise for material distribution
                    float materialNoise = glm::simplex(glm::vec2(worldX * 0.1f, worldZ * 0.1f));
                    float depthNoise = (materialNoise + 1.0f) * 0.5f; // 0 to 1
                    
                    // Surface layer with biome influence
                    if (y == localTerrainHeight - 1) {
                        // Higher elevations tend to be more rocky
                        if (height > m_params.maxHeight * 0.7f && depthNoise > 0.5f) {
                            // Mountain or high terrain
                            chunk->setBlock(x, y, z, BlockType::STONE);
                        } 
                        // Desert biome (hot, dry)
                        else if (temperature > 0.7f && humidity < 0.35f) {
                            chunk->setBlock(x, y, z, BlockType::SAND);
                        }
                        // Near water areas tend to be sandy
                        else if (height < m_params.maxHeight * 0.3f && humidity > 0.4f) {
                            chunk->setBlock(x, y, z, BlockType::SAND);
                        } 
                        // Most areas are grassy
                        else {
                            // Grass with more varied distribution
                            chunk->setBlock(x, y, z, BlockType::GRASS);
                        }
                    } 
                    // Subsurface layers
                    else {
                        // Depth of topsoil varies with humidity and noise
                        int dirtDepth = 2 + static_cast<int>(humidity * 3.0f + depthNoise * 2.0f);
                        
                        if (y >= localTerrainHeight - dirtDepth) {
                            // Near-surface layer is dirt with variable depth
                        chunk->setBlock(x, y, z, BlockType::DIRT);
                    } else {
                            // Deeper layer is stone, with occasional dirt pockets
                            if (depthNoise > 0.8f && height < m_params.maxHeight * 0.6f) {
                                chunk->setBlock(x, y, z, BlockType::DIRT); // Dirt pocket
                            } else {
                        chunk->setBlock(x, y, z, BlockType::STONE);
                            }
                        }
                    }
                } else if (height < m_params.maxHeight * 0.35f) {
                    // Add water in low areas below a certain height
                    // Deeper water in lower areas
                    float waterThreshold = m_params.maxHeight * 0.25f + m_params.maxHeight * 0.1f * humidity;
                    if (height < waterThreshold) {
                        chunk->setBlock(x, y, z, BlockType::WATER);
                    }
                }
                // Everything else remains AIR by default
            }
        }
    }
    
    Logger::logDebug("Stylized 'Pelican Harbor' terrain generated for chunk at (" + 
                    std::to_string(chunkPos.x) + ", " +
                    std::to_string(chunkPos.y) + ", " +
                    std::to_string(chunkPos.z) + ")");
}

void VoxelWorld::generateFeatures(Chunk* chunk) {
    if (chunk == nullptr) {
        return;
    }
    
    // Get chunk position
    const glm::ivec3& chunkPos = chunk->getPosition();
    
    // Skip below-ground and high-up chunks for trees
    if (chunkPos.y < 0 || chunkPos.y > 2) {
        return;
    }
    
    // Seed for this chunk
    int chunkSeed = m_params.seed + 
                    chunkPos.x * 10000 + 
                    chunkPos.y * 1000 + 
                    chunkPos.z * 100;
    
    std::mt19937 rng(chunkSeed);
    std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    std::uniform_int_distribution<int> randPos(3, CHUNK_SIZE - 4); // Keep away from edges
    
    // Enhanced feature generation chances for "Pelican Harbor" aesthetic
    float treeChance = 0.20f;     // Adjusted from 0.30f
    float rockChance = 0.10f;     // Adjusted from 0.15f
    float flowerChance = 0.30f;   // Adjusted from 0.40f
    float mushroomChance = 0.15f; // Adjusted from 0.20f
    float bushChance = 0.20f;     // Adjusted from 0.25f
    
    // Note: randPosTree is now defined inside the loop as trunkBaseSideLength varies per tree

    // Generate trees - more whimsical and varied for stylized aesthetic
    if (rand01(rng) < treeChance) {
        int numTrees = 1 + static_cast<int>(rand01(rng) * 2.0f); // Reduced numTrees due to their larger size
        
        for (int i = 0; i < numTrees; i++) {
            int currentTrunkBaseSideLength = 5 + static_cast<int>(rand01(rng) * 5.0f); // 5 to 9 voxels (1.25m to 2.25m diameter)
            std::uniform_int_distribution<int> randPosTree(3, CHUNK_SIZE - (3 + currentTrunkBaseSideLength -1) );

            // Random position for the tree (base corner of the NxN trunk)
            int treeX = randPosTree(rng);
            int treeZ = randPosTree(rng);
            
            // Find the ground level
            int groundY = -1;
            // Check ground for the NxN area
            bool canPlaceTrunk = true;
            int minGroundY = CHUNK_SIZE;
            for (int dx = 0; dx < currentTrunkBaseSideLength; ++dx) { 
                for (int dz = 0; dz < currentTrunkBaseSideLength; ++dz) { 
                    int currentGroundY = -1;
                    for (int y = CHUNK_SIZE - 1; y >= 0; y--) {
                        BlockType block = chunk->getBlock(treeX + dx, y, treeZ + dz);
                        if (block != BlockType::AIR && block != BlockType::WATER) {
                            currentGroundY = y;
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
            groundY = minGroundY; // Use the lowest ground Y of the NxN area
            
            // Only place tree if we found ground and it's grass for the entire NxN base
            if (canPlaceTrunk && groundY >= 0) {
                // Trunk height: 30-50 voxels (7.5m to 12.5m)
                int trunkHeight = 30 + static_cast<int>(rand01(rng) * 21.0f); 
                float trunkRadius = (currentTrunkBaseSideLength - 1) / 2.0f;
                float trunkCenterX = (currentTrunkBaseSideLength - 1) / 2.0f; // Relative to local 0,0 of the base square
                float trunkCenterZ = (currentTrunkBaseSideLength - 1) / 2.0f; // Relative to local 0,0 of the base square

                // Generate circular trunk
                for (int y = 1; y <= trunkHeight; y++) {
                    if (groundY + y >= CHUNK_SIZE) break; 
                    for (int dz_local = 0; dz_local < currentTrunkBaseSideLength; ++dz_local) { 
                        for (int dx_local = 0; dx_local < currentTrunkBaseSideLength; ++dx_local) { 
                            float distFromTrunkCenter = std::sqrt(
                                (dx_local - trunkCenterX) * (dx_local - trunkCenterX) +
                                (dz_local - trunkCenterZ) * (dz_local - trunkCenterZ)
                            );
                            if (distFromTrunkCenter <= trunkRadius + 0.5f) { 
                                if (treeX + dx_local < CHUNK_SIZE && treeZ + dz_local < CHUNK_SIZE) { 
                                    chunk->setBlock(treeX + dx_local, groundY + y, treeZ + dz_local, BlockType::WOOD);
                                }
                            }
                        }
                    }
                }
                
                // Create stylized foliage, larger and adapted for variable width trunk
                int leavesStart = trunkHeight - 8; 
                int leavesEnd = trunkHeight + 20 + static_cast<int>(rand01(rng) * 16.0f);  
                
                bool isRoundTop = rand01(rng) > 0.4f;
                float foliageCenterX = treeX + (currentTrunkBaseSideLength - 1) / 2.0f; 
                float foliageCenterZ = treeZ + (currentTrunkBaseSideLength - 1) / 2.0f; 

                for (int ly = leavesStart; ly <= leavesEnd; ly++) {
                    if (groundY + ly < 0 || groundY + ly >= CHUNK_SIZE) continue;

                    float heightPosition = static_cast<float>(ly - leavesStart) / (leavesEnd - leavesStart);
                    heightPosition = std::max(0.0f, std::min(1.0f, heightPosition)); 
                    
                    int baseLeafRadius;
                    if (isRoundTop) {
                        baseLeafRadius = 18 + static_cast<int>(7.0f * sin(heightPosition * 3.14159f)); 
                    } else {
                        baseLeafRadius = 25 - static_cast<int>(10.0f * heightPosition); 
                    }
                    baseLeafRadius = std::max(1, baseLeafRadius); 
                    
                    for (int lz_offset = -baseLeafRadius; lz_offset <= baseLeafRadius; lz_offset++) {
                        for (int lx_offset = -baseLeafRadius; lx_offset <= baseLeafRadius; lx_offset++) {
                            
                            int blockX = static_cast<int>(foliageCenterX + lx_offset);
                            int blockY = groundY + ly;
                            int blockZ = static_cast<int>(foliageCenterZ + lz_offset);

                            // Skip if trying to place leaves inside the NxN trunk area
                            if (ly < leavesEnd - 3 && 
                                blockX >= treeX && blockX < treeX + currentTrunkBaseSideLength && 
                                blockZ >= treeZ && blockZ < treeZ + currentTrunkBaseSideLength) {
                                 if (ly < trunkHeight + (currentTrunkBaseSideLength > 3 ? 2 : 1) ) continue; 
                            }
                            
                            float dist = std::sqrt(lx_offset*lx_offset + lz_offset*lz_offset);
                            if (dist > baseLeafRadius + (rand01(rng) * 1.0f - 0.5f)) continue; 
                            if (dist > baseLeafRadius - 1.5f && rand01(rng) < 0.30f) continue; 
                            
                            if (blockX < 0 || blockX >= CHUNK_SIZE ||
                                blockZ < 0 || blockZ >= CHUNK_SIZE) {
                                continue;
                            }
                            if (chunk->getBlock(blockX, blockY, blockZ) != BlockType::AIR) {
                                continue;
                            }
                            chunk->setBlock(blockX, blockY, blockZ, BlockType::LEAVES);
                        }
                    }
                }
            }
        }
    }
    
    // Generate small rocks and boulders - varied shapes for the stylized aesthetic
    if (rand01(rng) < rockChance) {
        int numRocks = 1 + static_cast<int>(rand01(rng) * 3.0f);
        
        for (int i = 0; i < numRocks; i++) {
            // Random position for the rock
            int rockX = randPos(rng);
            int rockZ = randPos(rng);
            
            // Find the ground level
            int groundY = -1;
            for (int y = CHUNK_SIZE - 1; y >= 0; y--) {
                BlockType block = chunk->getBlock(rockX, y, rockZ);
                if (block != BlockType::AIR && block != BlockType::WATER) {
                    groundY = y;
                    break;
                }
            }
            
            // Only place rock if we found ground
            if (groundY >= 0) {
                // Rock size and style varies
                bool isFlat = rand01(rng) > 0.7f;
                int rockHeight = isFlat ? 1 : 1 + static_cast<int>(rand01(rng) * 2.0f);
                int rockRadius = 1 + static_cast<int>(rand01(rng) * 2.2f);
                
                // Create boulder shape with slightly asymmetrical design
                for (int ry = 0; ry < rockHeight; ry++) {
                    // Flat rocks have larger radius, tall rocks taper more
                    float heightFactor = isFlat ? 0.3f : (static_cast<float>(ry) / rockHeight);
                    int layerRadius = std::max(1, static_cast<int>(rockRadius * (1.0f - heightFactor * 0.7f)));
                    
                    for (int rz = -layerRadius; rz <= layerRadius; rz++) {
                        for (int rx = -layerRadius; rx <= layerRadius; rx++) {
                            // Create a slightly irregular shape
                            float distortion = (rx * 74.21f + rz * 45.13f) * 0.01f;
                            float dist = std::sqrt(rx*rx + rz*rz) + (distortion - floor(distortion));
                            if (dist > layerRadius * (1.0f + (rand01(rng) * 0.2f - 0.1f))) continue;
                            
                            // Calculate absolute position
                            int blockX = rockX + rx;
                            int blockY = groundY + ry + 1; // Place on top of ground
                            int blockZ = rockZ + rz;
                            
                            // Skip if out of bounds
                            if (blockX < 0 || blockX >= CHUNK_SIZE ||
                                blockY < 0 || blockY >= CHUNK_SIZE ||
                                blockZ < 0 || blockZ >= CHUNK_SIZE) {
                                continue;
                            }
                            
                            // Skip if not air (don't overwrite other blocks)
                            if (chunk->getBlock(blockX, blockY, blockZ) != BlockType::AIR) {
                                continue;
                            }
                            
                            // Add stone block for the rock
                            chunk->setBlock(blockX, blockY, blockZ, BlockType::STONE);
                        }
                    }
                }
            }
        }
    }
    
    // Generate colorful flower patches - for vibrant "Pelican Harbor" style
    if (rand01(rng) < flowerChance) {
        int numFlowerPatches = 1 + static_cast<int>(rand01(rng) * 3.0f);
        
        for (int i = 0; i < numFlowerPatches; i++) {
            // Center of the flower patch
            int patchX = randPos(rng);
            int patchZ = randPos(rng);
            
            // Size of the patch
            int patchRadius = 1 + static_cast<int>(rand01(rng) * 3.0f);
            
            // Number of flowers in this patch
            int numFlowers = 3 + static_cast<int>(rand01(rng) * patchRadius * 3.0f);
            
            for (int f = 0; f < numFlowers; f++) {
                // Random position within patch radius (scattered)
                float angle = rand01(rng) * 6.28318f; // Random angle in radians
                float distance = rand01(rng) * patchRadius;
                int flowerX = patchX + static_cast<int>(cos(angle) * distance);
                int flowerZ = patchZ + static_cast<int>(sin(angle) * distance);
                
                // Keep within chunk bounds
                flowerX = std::max(0, std::min(flowerX, CHUNK_SIZE - 1));
                flowerZ = std::max(0, std::min(flowerZ, CHUNK_SIZE - 1));
                
                // Find ground level
                int groundY = -1;
                for (int y = CHUNK_SIZE - 1; y >= 0; y--) {
                    BlockType block = chunk->getBlock(flowerX, y, flowerZ);
                    if (block != BlockType::AIR && block != BlockType::WATER) {
                        groundY = y;
                        break;
                    }
                }
                
                // Only place flower on grass
                if (groundY >= 0 && chunk->getBlock(flowerX, groundY, flowerZ) == BlockType::GRASS) {
                    // Future enhancement: Add specific flower block types instead of colored leaves
                    // For now, use leaves of certain colors to represent flowers
                    chunk->setBlock(flowerX, groundY + 1, flowerZ, BlockType::LEAVES);
                }
            }
        }
    }
    
    // Generate mushrooms - scattered on forest floor for charm and visual interest
    if (rand01(rng) < mushroomChance) {
        int numMushrooms = 2 + static_cast<int>(rand01(rng) * 4.0f);
        
        for (int i = 0; i < numMushrooms; i++) {
            // Random position for mushroom
            int mushroomX = randPos(rng);
            int mushroomZ = randPos(rng);
            
            // Find ground level
            int groundY = -1;
            for (int y = CHUNK_SIZE - 1; y >= 0; y--) {
                BlockType block = chunk->getBlock(mushroomX, y, mushroomZ);
                if (block != BlockType::AIR && block != BlockType::WATER) {
                    groundY = y;
                    break;
                }
            }
            
            // Only place mushroom on dirt, forest floor
            if (groundY >= 0 && 
                (chunk->getBlock(mushroomX, groundY, mushroomZ) == BlockType::DIRT || 
                 chunk->getBlock(mushroomX, groundY, mushroomZ) == BlockType::GRASS)) {
                
                // Simple mushroom - stem and cap (currently using wood and leaves as placeholders)
                // Stem
                chunk->setBlock(mushroomX, groundY + 1, mushroomZ, BlockType::WOOD);
                
                // Cap (simple cross shape)
                if (groundY + 2 < CHUNK_SIZE) {
                    chunk->setBlock(mushroomX, groundY + 2, mushroomZ, BlockType::LEAVES);
                    
                    // Extend cap in 4 directions
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
    
    // Generate small bushes - adds visual variety and detail
    if (rand01(rng) < bushChance) {
        int numBushes = 2 + static_cast<int>(rand01(rng) * 3.0f);
        
        for (int i = 0; i < numBushes; i++) {
            // Random position for bush
            int bushX = randPos(rng);
            int bushZ = randPos(rng);
            
            // Find ground level
            int groundY = -1;
            for (int y = CHUNK_SIZE - 1; y >= 0; y--) {
                BlockType block = chunk->getBlock(bushX, y, bushZ);
                if (block != BlockType::AIR && block != BlockType::WATER) {
                    groundY = y;
                    break;
                }
            }
            
            // Only place bush on grass
            if (groundY >= 0 && chunk->getBlock(bushX, groundY, bushZ) == BlockType::GRASS) {
                // Bush height
                int bushHeight = 1 + static_cast<int>(rand01(rng) * 2.0f);
                
                // Bush shape
                for (int by = 0; by < bushHeight; by++) {
                    // Skip if out of bounds
                    if (groundY + by + 1 >= CHUNK_SIZE) continue;
                    
                    int bushRadius = 1;
                    if (by == 0 && bushHeight > 1) bushRadius = 2;
                    
                    for (int bz = -bushRadius; bz <= bushRadius; bz++) {
                        for (int bx = -bushRadius; bx <= bushRadius; bx++) {
                            // Create round bush shape
                            float dist = std::sqrt(bx*bx + bz*bz);
                            if (dist > bushRadius) continue;
                            
                            // Skip randomly for more natural, irregular shape
                            if (dist > bushRadius - 0.5f && rand01(rng) < 0.4f) continue;
                            
                            // Calculate absolute position
                            int blockX = bushX + bx;
                            int blockY = groundY + by + 1;
                            int blockZ = bushZ + bz;
                            
                            // Skip if out of bounds
                            if (blockX < 0 || blockX >= CHUNK_SIZE ||
                                blockY < 0 || blockY >= CHUNK_SIZE ||
                                blockZ < 0 || blockZ >= CHUNK_SIZE) {
                                continue;
                            }
                            
                            // Skip if not air
                            if (chunk->getBlock(blockX, blockY, blockZ) != BlockType::AIR) {
                                continue;
                            }
                            
                            // Add bush block (using leaves for now)
                            chunk->setBlock(blockX, blockY, blockZ, BlockType::LEAVES);
                        }
                    }
                }
            }
        }
    }
    
    Logger::logDebug("Stylized 'Pelican Harbor' features generated for chunk at (" + 
                    std::to_string(chunkPos.x) + ", " +
                    std::to_string(chunkPos.y) + ", " +
                    std::to_string(chunkPos.z) + ")");
}

void VoxelWorld::updateChunkMeshes(const glm::ivec3& centerPos) {
    // For each chunk in the view distance
    for (auto& pair : m_chunks) {
        const glm::ivec3& chunkPos = pair.first;
        Chunk* chunk = pair.second.get();
        
        // Calculate distance from center
        glm::ivec3 diff = chunkPos - centerPos;
        float distance = sqrt(diff.x * diff.x + diff.z * diff.z);
        
        // Skip chunks outside view distance
        if (distance > m_viewDistanceInChunks) {
            continue;
        }
        
        // Get pointers to the 6 surrounding chunks (+X, -X, +Y, -Y, +Z, -Z)
        const Chunk* surroundingChunks[6] = {
            getChunk(chunkPos + glm::ivec3(1, 0, 0)),
            getChunk(chunkPos + glm::ivec3(-1, 0, 0)),
            getChunk(chunkPos + glm::ivec3(0, 1, 0)),
            getChunk(chunkPos + glm::ivec3(0, -1, 0)),
            getChunk(chunkPos + glm::ivec3(0, 0, 1)),
            getChunk(chunkPos + glm::ivec3(0, 0, -1))
        };
        
        // Generate mesh for this chunk
        chunk->generateMesh(surroundingChunks);
    }
}

BlockType VoxelWorld::getBlockAt(const glm::vec3& worldPos) const {
    // Convert to chunk and local coordinates
    glm::ivec3 chunkPos = Chunk::worldToChunkPos(worldPos);
    glm::ivec3 localPos = Chunk::worldToLocalPos(worldPos);
    
    // Get the chunk
    Chunk* chunk = getChunk(chunkPos);
    if (chunk == nullptr) {
        return BlockType::AIR; // Assume air for non-existent chunks
    }
    
    // Get the block from the chunk
    return chunk->getBlock(localPos.x, localPos.y, localPos.z);
}

void VoxelWorld::setBlockAt(const glm::vec3& worldPos, BlockType type) {
    // Convert to chunk and local coordinates
    glm::ivec3 chunkPos = Chunk::worldToChunkPos(worldPos);
    glm::ivec3 localPos = Chunk::worldToLocalPos(worldPos);
    
    // Get or create the chunk
    Chunk* chunk = getChunk(chunkPos);
    if (chunk == nullptr) {
        chunk = createChunk(chunkPos);
    }
    
    // Set the block in the chunk
    chunk->setBlock(localPos.x, localPos.y, localPos.z, type);
    
    // Check if the modified block is on a boundary of the chunk.
    // If so, the neighboring chunk also needs its mesh updated.
    const glm::ivec3 neighborOffsets[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };
    bool onBoundary = false;
    int boundaryDirection = -1;

    if (localPos.x == 0) { onBoundary = true; boundaryDirection = 1; } // Touches -X neighbor
    else if (localPos.x == CHUNK_SIZE - 1) { onBoundary = true; boundaryDirection = 0; } // Touches +X neighbor
    if (localPos.y == 0) { onBoundary = true; boundaryDirection = 3; } // Touches -Y neighbor (can overwrite if multiple boundaries touched)
    else if (localPos.y == CHUNK_SIZE - 1) { onBoundary = true; boundaryDirection = 2; } // Touches +Y neighbor
    if (localPos.z == 0) { onBoundary = true; boundaryDirection = 5; } // Touches -Z neighbor
    else if (localPos.z == CHUNK_SIZE - 1) { onBoundary = true; boundaryDirection = 4; } // Touches +Z neighbor
    
    // A single block can be on multiple boundaries. We need to check all relevant neighbors.
    if (localPos.x == 0) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[1]); // -X neighbor
        if (neighbor) neighbor->markModified();
    }
    if (localPos.x == CHUNK_SIZE - 1) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[0]); // +X neighbor
        if (neighbor) neighbor->markModified();
    }
    if (localPos.y == 0) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[3]); // -Y neighbor
        if (neighbor) neighbor->markModified();
    }
    if (localPos.y == CHUNK_SIZE - 1) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[2]); // +Y neighbor
        if (neighbor) neighbor->markModified();
    }
    if (localPos.z == 0) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[5]); // -Z neighbor
        if (neighbor) neighbor->markModified();
    }
    if (localPos.z == CHUNK_SIZE - 1) {
        Chunk* neighbor = getChunk(chunkPos + neighborOffsets[4]); // +Z neighbor
        if (neighbor) neighbor->markModified();
    }

    // No need to call updateChunkMeshes here directly. 
    // The main loop's call to updateChunkMeshes will handle it because chunks are marked modified.
}

float VoxelWorld::getHeightAt(float x, float z) const {
    // Use higher precision for micro-voxel terrain
    // Start from a high position and move down with smaller steps
    float maxHeight = 128.0f; // Maximum world height in blocks
    float heightIncrement = m_params.cellSize; // Step down by one micro-voxel at a time
    
    for (float y = maxHeight; y >= 0.0f; y -= heightIncrement) {
        glm::vec3 pos(x, y, z);
        BlockType block = getBlockAt(pos);
        
        if (block != BlockType::AIR && BlockData::isSolid(block)) {
            // Return the height of the block top
            // Add cell size to get the top surface
            return y + m_params.cellSize;
        }
    }
    
    // Fallback to 0 if no solid block found
    return 0.0f;
}

bool VoxelWorld::checkCollision(const Player& player) const {
    // Get player position and size
    Vec3 playerPos = player.getPosition();
    float playerSize = player.getCollisionRadius();
    
    // Clear previous debug points if debug is enabled
    if (m_collisionDebugEnabled) {
        m_collisionDebugPoints.clear();
    }
    
    // Calculate step size based on cell size for proper micro-voxel sampling
    // Use a fraction of cellSize to ensure thorough collision checking
    float stepSize = m_params.cellSize * 0.5f;
    
    // Ensure we have a reasonable step size (not too small, not too large)
    stepSize = std::max(stepSize, 0.05f);
    
    bool collision = false;
    
    // Check blocks in player's bounding box with appropriate sampling density
    for (float y = playerPos.y; y <= playerPos.y + player.getHeight(); y += stepSize) {
        for (float z = playerPos.z - playerSize; z <= playerPos.z + playerSize; z += stepSize) {
            for (float x = playerPos.x - playerSize; x <= playerPos.x + playerSize; x += stepSize) {
                BlockType block = getBlockAt(glm::vec3(x, y, z));
                
                if (block != BlockType::AIR && BlockData::isSolid(block)) {
                    // Store collision point for debug visualization
                    if (m_collisionDebugEnabled) {
                        m_collisionDebugPoints.push_back(glm::vec3(x, y, z));
                    }
                    
                    collision = true;
                    
                    // If not in debug mode, return immediately on first collision
                    if (!m_collisionDebugEnabled) {
                        return true;
                    }
                }
            }
        }
    }
    
    return collision;
}

void VoxelWorld::setWorldSize(int size) {
    m_worldSizeInChunks = size;
}

void VoxelWorld::renderCollisionDebug(const Camera& camera, const Player& player) {
    if (!m_collisionDebugEnabled || m_collisionDebugPoints.empty()) {
        return;
    }
    
    // Check if we need to initialize debug rendering resources
    if (m_debugVAO == 0) {
        glGenVertexArrays(1, &m_debugVAO);
        glGenBuffers(1, &m_debugVBO);
        
        glBindVertexArray(m_debugVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_debugVBO);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    // Create a simple shader program for rendering debug points if m_shader is not available
    static Shader* debugShader = nullptr;
    if (debugShader == nullptr) {
        try {
            debugShader = new Shader("assets/shaders/debug.vert", "assets/shaders/debug.frag");
        } catch (const std::exception& e) {
            Logger::logError("Failed to create debug shader: " + std::string(e.what()));
            return;
        }
    }
    
    // Update buffer with collision points
    glBindBuffer(GL_ARRAY_BUFFER, m_debugVBO);
    glBufferData(GL_ARRAY_BUFFER, m_collisionDebugPoints.size() * sizeof(glm::vec3), 
                m_collisionDebugPoints.data(), GL_DYNAMIC_DRAW);
    
    // Render points
    debugShader->use();
    debugShader->setMat4("view", camera.getViewMatrix());
    debugShader->setMat4("projection", camera.getProjectionMatrix(16.0f / 9.0f)); // TODO: Get actual aspect ratio
    debugShader->setVec4("color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red points
    
    // Point size for visibility, configurable
    float pointSize = Config::getFloat("Debug.collision_point_size", 5.0f);
    glPointSize(pointSize);
    
    // Disable depth test temporarily to make points visible through blocks
    GLboolean depthEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
    glDisable(GL_DEPTH_TEST);
    
    glBindVertexArray(m_debugVAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_collisionDebugPoints.size()));
    glBindVertexArray(0);
    
    // Restore depth test state
    if (depthEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
    
    // Reset point size
    glPointSize(1.0f);
}

} // namespace Sylva 
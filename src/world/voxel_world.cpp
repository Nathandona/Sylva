#include "voxel_world.h"
#include "core/config.h"
#include "core/logger.h"
#include "renderer/camera.h"
#include "renderer/frustum.h"
#include "renderer/shader.h"
#include "player.h"
#include <glad/glad.h>
#include <glm/gtc/noise.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include "generation/terrain_generator.h"

namespace Sylva {

VoxelWorld::VoxelWorld() {
    // Load world parameters from config if available
    m_params.terrainSize = Config::getInt("World.terrain_size", m_params.terrainSize);
    m_params.cellSize = Config::getFloat("World.cell_size", m_params.cellSize);
    m_params.maxHeight = Config::getFloat("World.max_height", m_params.maxHeight);
    m_params.seed = Config::getInt("World.seed", m_params.seed);
    m_worldSizeInChunks = Config::getInt("World.size_in_chunks", m_worldSizeInChunks);
    m_viewDistanceInChunks = Config::getInt("World.view_distance", m_viewDistanceInChunks);
    m_chunkYMin = Config::getInt("World.chunk_y_min", m_chunkYMin);
    m_chunkYMax = Config::getInt("World.chunk_y_max", m_chunkYMax);

    m_params.noiseScale = Config::getFloat("World.noise_scale", 0.05f);
    m_params.detailScale = Config::getFloat("World.detail_scale", 0.20f);
    m_params.terrainSmoothness = Config::getFloat("World.terrain_smoothness", 0.6f);
    m_params.plateauWidthVoxels = Config::getInt("World.plateau_width_voxels", m_params.plateauWidthVoxels);

    m_terrainGenerator = std::make_unique<TerrainGenerator>(m_params);
    m_jobs = std::make_unique<ChunkJobSystem>();

    Logger::logInfo("Micro-voxel world created with size " + std::to_string(m_worldSizeInChunks) +
                    " chunks and view distance " + std::to_string(m_viewDistanceInChunks) + " chunks");
}

VoxelWorld::VoxelWorld(const WorldParams& params) : m_params(params) {
    m_worldSizeInChunks = Config::getInt("World.size_in_chunks", m_worldSizeInChunks);
    m_viewDistanceInChunks = Config::getInt("World.view_distance", m_viewDistanceInChunks);
    m_chunkYMin = Config::getInt("World.chunk_y_min", m_chunkYMin);
    m_chunkYMax = Config::getInt("World.chunk_y_max", m_chunkYMax);
    m_terrainGenerator = std::make_unique<TerrainGenerator>(m_params);
    m_jobs = std::make_unique<ChunkJobSystem>();

    Logger::logInfo("VoxelWorld created with custom parameters");
}

VoxelWorld::~VoxelWorld() {
    // Join workers BEFORE destroying chunks / generators they reference.
    m_jobs.reset();

    if (m_debugVAO != 0) {
        glDeleteVertexArrays(1, &m_debugVAO);
        m_debugVAO = 0;
    }
    if (m_debugVBO != 0) {
        glDeleteBuffers(1, &m_debugVBO);
        m_debugVBO = 0;
    }
    // m_shader / m_debugShader / m_chunks cleaned up by unique_ptr / map dtors.
}

bool VoxelWorld::initializeGraphics() {
    Logger::logInfo("Initializing micro-voxel world graphics");

    // Initialize block data
    BlockData::initialize();

    // Create shader
    return createShader();
}

bool VoxelWorld::createShader() {
    try {
        m_shader = std::make_unique<Shader>("assets/shaders/voxel.vert", "assets/shaders/voxel.frag");
        Logger::logInfo("Voxel shader created successfully");
        return true;
    } catch (const std::exception& e) {
        Logger::logError("Failed to create voxel shader: " + std::string(e.what()));
        return false;
    }
}

void VoxelWorld::initializeWorldChunks(const glm::ivec3& centerPos) {
    // Async — enqueue every chunk in range; main thread keeps moving.
    for (int y = m_chunkYMin; y <= m_chunkYMax; y++) {
        for (int z = -m_worldSizeInChunks; z <= m_worldSizeInChunks; z++) {
            for (int x = -m_worldSizeInChunks; x <= m_worldSizeInChunks; x++) {
                loadChunkAsync(centerPos + glm::ivec3(x, y, z));
            }
        }
    }
}

void VoxelWorld::loadChunkSync(const glm::ivec3& chunkPos) {
    if (getChunk(chunkPos) != nullptr)
        return;
    Chunk* chunk = createChunk(chunkPos);
    chunk->setState(ChunkState::Generating);
    generateChunkTerrain(chunk);
    generateChunkFeatures(chunk);
    chunk->setState(ChunkState::Ready);
}

void VoxelWorld::loadChunkAsync(const glm::ivec3& chunkPos) {
    if (getChunk(chunkPos) != nullptr)
        return;
    Chunk* chunk = createChunk(chunkPos); // main-thread map insert; chunk in Pending
    if (!m_jobs) {
        // Fallback: pool not constructed yet (shouldn't happen post-ctor).
        chunk->setState(ChunkState::Generating);
        generateChunkTerrain(chunk);
        generateChunkFeatures(chunk);
        chunk->setState(ChunkState::Ready);
        return;
    }
    TerrainGenerator* gen = m_terrainGenerator.get();
    m_jobs->submit([chunk, gen]() {
        chunk->setState(ChunkState::Generating);
        gen->generateTerrain(chunk);
        gen->generateFeatures(chunk);
        // Release: makes all m_blocks writes visible to any thread that
        // does an acquire-load and sees Ready.
        chunk->setState(ChunkState::Ready);
    });
}

void VoxelWorld::generateWorld(const glm::vec3& playerPosition) {
    Logger::logInfo("Generating micro-voxel world around player position (" + std::to_string(playerPosition.x) + ", " +
                    std::to_string(playerPosition.y) + ", " + std::to_string(playerPosition.z) + ")");
    glm::ivec3 const centerChunkPos = Chunk::worldToChunkPos(playerPosition);

    // Sync-load player-local chunks so the player spawns on solid ground and
    // collision/getBlockAt see real terrain immediately. Manhattan-1 in XZ
    // across the full Y range gives a 3x3 column of chunks under/around the
    // player — enough for footing and basic look-ahead.
    for (int y = m_chunkYMin; y <= m_chunkYMax; y++) {
        for (int z = -1; z <= 1; z++) {
            for (int x = -1; x <= 1; x++) {
                loadChunkSync(centerChunkPos + glm::ivec3(x, y, z));
            }
        }
    }

    // Everything else streams in via worker pool — no startup freeze.
    initializeWorldChunks(centerChunkPos);

    // Mesh the chunks already Ready (the sync-loaded ring). Async ones will
    // be picked up by updateChunkMeshes() each frame as their state flips.
    updateChunkMeshes(centerChunkPos);
    Logger::logInfo("Micro-voxel world generation kicked off (sync ring meshed; rest streaming)");
}

void VoxelWorld::generateChunkTerrain(Chunk* chunk) {
    if (chunk == nullptr) {
        Logger::logWarning("Attempted to generate terrain for null chunk");
        return;
    }
    m_terrainGenerator->generateTerrain(chunk);
}

void VoxelWorld::generateChunkFeatures(Chunk* chunk) {
    if (chunk == nullptr) {
        Logger::logWarning("Attempted to generate features for null chunk");
        return;
    }
    m_terrainGenerator->generateFeatures(chunk);
}

bool VoxelWorld::updateChunkVisibility(const glm::ivec3& chunkPos, const glm::ivec3& playerChunkPos) const {
    glm::ivec3 const diff = chunkPos - playerChunkPos;
    float const distance = sqrt(diff.x * diff.x + diff.z * diff.z);
    return distance <= m_viewDistanceInChunks;
}

bool VoxelWorld::updatePlayerChunkPosition(const glm::ivec3& playerChunkPos, glm::ivec3& lastPlayerChunkPos) const {
    if (playerChunkPos != lastPlayerChunkPos) {
        lastPlayerChunkPos = playerChunkPos;
        return true;
    }
    return false;
}

void VoxelWorld::updateChunkLoading(const glm::ivec3& playerChunkPos) {
    for (int y = m_chunkYMin; y <= m_chunkYMax; y++) {
        for (int z = -m_viewDistanceInChunks; z <= m_viewDistanceInChunks; z++) {
            for (int x = -m_viewDistanceInChunks; x <= m_viewDistanceInChunks; x++) {
                glm::ivec3 const chunkPos = playerChunkPos + glm::ivec3(x, y, z);
                if (updateChunkVisibility(chunkPos, playerChunkPos)) {
                    loadChunkAsync(chunkPos);
                }
            }
        }
    }
    updateChunkMeshes(playerChunkPos);
}

void VoxelWorld::update(float /*deltaTime*/, const glm::vec3& playerPosition) {
    glm::ivec3 const playerChunkPos = Chunk::worldToChunkPos(playerPosition);
    static glm::ivec3 lastPlayerChunkPos(-999999, -999999, -999999);

    if (updatePlayerChunkPosition(playerChunkPos, lastPlayerChunkPos)) {
        updateChunkLoading(playerChunkPos);
    } else {
        // Player didn't cross a chunk boundary, but async workers may have
        // finished new chunks since last frame — drain them.
        updateChunkMeshes(playerChunkPos);
    }
}

void VoxelWorld::render(const Camera& camera, float aspectRatio) {
    if (!m_shader) {
        Logger::logWarning("Cannot render voxel world, shader not initialized");
        return;
    }
    glm::mat4 const viewMatrix = camera.getViewMatrix();
    glm::mat4 const projectionMatrix = camera.getProjectionMatrix(aspectRatio);
    glm::vec3 const lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    m_shader->use();

    // Per-frame uniforms — set ONCE here, not per chunk. view/projection/
    // viewPos / lighting / fog don't change between chunks; setting them in
    // Chunk::render meant N redundant uniform writes per frame.
    m_shader->setMat4("view", viewMatrix);
    m_shader->setMat4("projection", projectionMatrix);
    glm::mat4 invView = glm::inverse(viewMatrix);
    auto const cameraPos = glm::vec3(invView[3]);
    m_shader->setVec3("viewPos", cameraPos);

    m_shader->setVec3("lightDir", lightDir);
    glm::vec3 const lightColor(Config::getFloat("Lighting.light_color.x", 1.0f),
                               Config::getFloat("Lighting.light_color.y", 0.95f),
                               Config::getFloat("Lighting.light_color.z", 0.8f));
    glm::vec3 const ambientColor(Config::getFloat("Lighting.ambient_color.x", 0.45f),
                                 Config::getFloat("Lighting.ambient_color.y", 0.45f),
                                 Config::getFloat("Lighting.ambient_color.z", 0.45f));
    m_shader->setVec3("uniformLightColor", lightColor);
    m_shader->setVec3("uniformAmbientColor", ambientColor);

    // Distance fog — match the GL clear color so far chunks blend with the
    // sky rather than reading as a visible cutoff plane.
    glm::vec3 const fogColor(Config::getFloat("Fog.color.x", 0.5f),
                             Config::getFloat("Fog.color.y", 0.7f),
                             Config::getFloat("Fog.color.z", 0.9f));
    const float fogStart = Config::getFloat("Fog.start", 12.0f);
    const float fogEnd = Config::getFloat("Fog.end", 22.0f);
    m_shader->setVec3("fogColor", fogColor);
    m_shader->setFloat("fogStart", fogStart);
    m_shader->setFloat("fogEnd", fogEnd);

    // Frustum cull — extract planes once, test each chunk's AABB before
    // issuing a draw call. With view_distance=3 we hold ~49 chunks; only
    // ~6-12 are typically in the camera frustum.
    Frustum frustum;
    frustum.extractFromMatrix(projectionMatrix * viewMatrix);

    const float cellSize = Config::getFloat("World.cell_size", 0.25f);
    const float chunkWorldSize = static_cast<float>(CHUNK_SIZE) * cellSize;

    int drawn = 0;
    for (const auto& pair : m_chunks) {
        const Chunk* chunk = pair.second.get();
        // Chunk::render also early-outs on !m_hasMesh / m_isEmpty, but the
        // explicit check here lets us skip the AABB test for known-empties.
        // Don't gate on ChunkState — mid-rebuild chunks should keep drawing
        // their previous mesh until the new upload completes (no blink).

        const glm::vec3 aabbMin(static_cast<float>(pair.first.x) * chunkWorldSize,
                                static_cast<float>(pair.first.y) * chunkWorldSize,
                                static_cast<float>(pair.first.z) * chunkWorldSize);
        const glm::vec3 aabbMax = aabbMin + glm::vec3(chunkWorldSize);
        if (!frustum.aabbVisible(aabbMin, aabbMax)) {
            continue;
        }

        pair.second->render(m_shader.get());
        ++drawn;
    }
    Logger::logDebug("Voxel world rendered " + std::to_string(drawn) + " / " +
                     std::to_string(m_chunks.size()) + " chunks");
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
    const glm::ivec3 neighborOffsets[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (const auto& offset : neighborOffsets) {
        Chunk* neighbor = getChunk(chunkPos + offset);
        if (neighbor) {
            neighbor->markModified();
        }
    }

    return chunkPtr;
}

namespace {
// Index into a flat 3x3x3 neighbor-pointer array. (0,0,0) = center = 13.
// Offsets each in {-1, 0, 1}.
constexpr int neighborIdx(int dx, int dy, int dz) {
    return ((dz + 1) * 9) + ((dy + 1) * 3) + (dx + 1);
}

// Sampler that resolves any chunk-local coord (incl. wraps into ±1 neighbor)
// against a pre-fetched 27-pointer Moore neighborhood — no map access. Built
// on the main thread, then captured by the worker's CPU mesh job.
BlockSampler makeNeighborhoodSampler(std::array<Chunk*, 27> neighborhood) {
    return [nb = neighborhood](int lx, int ly, int lz) -> BlockType {
        int dx = 0;
        int dy = 0;
        int dz = 0;
        auto wrap = [](int& local, int& coord) {
            while (local < 0) {
                local += CHUNK_SIZE;
                --coord;
            }
            while (local >= CHUNK_SIZE) {
                local -= CHUNK_SIZE;
                ++coord;
            }
        };
        wrap(lx, dx);
        wrap(ly, dy);
        wrap(lz, dz);
        // Outside the prefetched 3x3x3? Treat as AIR — face will reappear when
        // the missing neighbor lands and triggers a remesh.
        if (dx < -1 || dx > 1 || dy < -1 || dy > 1 || dz < -1 || dz > 1) {
            return BlockType::AIR;
        }
        Chunk* c = nb[neighborIdx(dx, dy, dz)];
        if (!c) {
            return BlockType::AIR;
        }
        return c->getBlock(lx, ly, lz);
    };
}
} // namespace

void VoxelWorld::updateChunkMeshes(const glm::ivec3& centerPos) {
    static constexpr glm::ivec3 kFaceOffsets[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0},
                                                   {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

    const int uploadBudget = std::max(1, Config::getInt("World.mesh_builds_per_frame", 2));
    const int dispatchBudget = std::max(1, Config::getInt("World.mesh_dispatches_per_frame", 8));

    // --- Phase A: upload BufferReady chunks (main thread, GL). -----------
    // Cheaper than full mesh build because the heavy CPU work is already
    // done — this is just glBufferData + glVertexAttribPointer. Still cap
    // it so a stack of finished jobs doesn't stall the frame.
    struct UploadCand {
        Chunk* chunk;
        glm::ivec3 pos;
        float dist;
    };
    std::vector<UploadCand> uploads;
    uploads.reserve(m_chunks.size());
    for (auto& pair : m_chunks) {
        if (pair.second->getState() != ChunkState::BufferReady) {
            continue;
        }
        glm::ivec3 const d = pair.first - centerPos;
        float const dist = sqrt(static_cast<float>(d.x * d.x + d.z * d.z));
        if (dist > m_viewDistanceInChunks) {
            continue;
        }
        uploads.push_back({pair.second.get(), pair.first, dist});
    }
    std::sort(uploads.begin(), uploads.end(),
              [](const UploadCand& a, const UploadCand& b) { return a.dist < b.dist; });

    int uploaded = 0;
    for (const auto& u : uploads) {
        if (uploaded >= uploadBudget) {
            break;
        }
        u.chunk->uploadPendingMesh();
        u.chunk->setState(ChunkState::Meshed);
        // Freshly meshed → adjacent Meshed chunks may need a re-mesh so their
        // shared face stops drawing AIR-assumed quads. Mark them dirty.
        for (const auto& off : kFaceOffsets) {
            if (Chunk* n = getChunk(u.pos + off)) {
                if (n->getState() == ChunkState::Meshed) {
                    n->markModified();
                }
            }
        }
        ++uploaded;
    }

    // --- Phase B: dispatch CPU mesh jobs for Ready chunks (and dirty Meshed
    // chunks that need a rebuild). Workers do the heavy AO/face work; main
    // thread just hands off the prefetched neighborhood and reads results
    // back via BufferReady → Phase A upload next frame.
    if (!m_jobs) {
        return;
    }

    struct DispatchCand {
        Chunk* chunk;
        glm::ivec3 pos;
        float dist;
    };
    std::vector<DispatchCand> dispatches;
    dispatches.reserve(m_chunks.size());
    for (auto& pair : m_chunks) {
        Chunk* chunk = pair.second.get();
        const ChunkState state = chunk->getState();

        const bool needsFirstMesh = (state == ChunkState::Ready);
        const bool needsRemesh = (state == ChunkState::Meshed && chunk->isModified());
        if (!needsFirstMesh && !needsRemesh) {
            continue;
        }

        glm::ivec3 const d = pair.first - centerPos;
        float const dist = sqrt(static_cast<float>(d.x * d.x + d.z * d.z));
        if (dist > m_viewDistanceInChunks) {
            continue;
        }
        dispatches.push_back({chunk, pair.first, dist});
    }
    std::sort(dispatches.begin(), dispatches.end(),
              [](const DispatchCand& a, const DispatchCand& b) { return a.dist < b.dist; });

    int dispatched = 0;
    for (const auto& d : dispatches) {
        if (dispatched >= dispatchBudget) {
            break;
        }

        // Defer dispatch until every face neighbor that actually exists in
        // the map has reached Ready. Otherwise the sampler would treat that
        // neighbor's blocks as AIR, and we'd draw bogus boundary faces +
        // wrong AO. Those would correct themselves once the neighbor lands
        // and triggers a re-mesh, but the brief seam shows up as a visible
        // grid of bright lines between chunks. Neighbors genuinely missing
        // from the map (edge of loaded world) don't block — they'll be
        // fixed by markModified when they later arrive.
        bool waitForNeighbor = false;
        for (const auto& off : kFaceOffsets) {
            Chunk* fn = getChunk(d.pos + off);
            if (fn && fn->getState() < ChunkState::Ready) {
                waitForNeighbor = true;
                break;
            }
        }
        if (waitForNeighbor) {
            continue;
        }

        // Prefetch the 27 neighbor chunk pointers on the main thread. Workers
        // never touch m_chunks, so an insert by the main thread can't race
        // with their reads. Only include neighbors whose state >= Ready so
        // m_blocks reads are safe.
        std::array<Chunk*, 27> nb{};
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    Chunk* c = getChunk(d.pos + glm::ivec3(dx, dy, dz));
                    if (c && c->getState() >= ChunkState::Ready) {
                        nb[neighborIdx(dx, dy, dz)] = c;
                    } else {
                        nb[neighborIdx(dx, dy, dz)] = nullptr;
                    }
                }
            }
        }

        // Mark this chunk MeshingCpu BEFORE dispatch so a subsequent frame
        // doesn't enqueue it again (idempotent dispatch).
        d.chunk->setState(ChunkState::MeshingCpu);
        // Clear the dirty bit pre-emptively. If another mutation happens
        // while the worker runs, m_isModified will be set again and Phase B
        // will pick the chunk up after it transitions back to Meshed.
        d.chunk->markModified(); // ensure true → cleared by uploadPendingMesh

        Chunk* chunk = d.chunk;
        BlockSampler sampler = makeNeighborhoodSampler(nb);
        m_jobs->submit([chunk, sampler = std::move(sampler)]() {
            chunk->buildMeshCpu(sampler);
            // Release-store: makes m_pendingMesh visible to the main thread
            // once it sees BufferReady via its acquire-load.
            chunk->setState(ChunkState::BufferReady);
        });
        ++dispatched;
    }
}

BlockType VoxelWorld::getBlockAt(const glm::vec3& worldPos) const {
    // Convert to chunk and local coordinates
    glm::ivec3 const chunkPos = Chunk::worldToChunkPos(worldPos);
    glm::ivec3 const localPos = Chunk::worldToLocalPos(worldPos);

    // Get the chunk
    Chunk* chunk = getChunk(chunkPos);
    if (chunk == nullptr) {
        return BlockType::AIR; // Assume air for non-existent chunks
    }

    // Don't read m_blocks while a worker may still be writing it. Acquire-load
    // pairs with the worker's release on Ready/Meshed; chunks below Ready are
    // treated as empty for queries (collision / sampler).
    if (chunk->getState() < ChunkState::Ready) {
        return BlockType::AIR;
    }

    // Get the block from the chunk
    return chunk->getBlock(localPos.x, localPos.y, localPos.z);
}

void VoxelWorld::setBlockAt(const glm::vec3& worldPos, BlockType type) {
    // Convert to chunk and local coordinates
    glm::ivec3 const chunkPos = Chunk::worldToChunkPos(worldPos);
    glm::ivec3 localPos = Chunk::worldToLocalPos(worldPos);

    // Get or create the chunk
    Chunk* chunk = getChunk(chunkPos);
    if (chunk == nullptr) {
        chunk = createChunk(chunkPos);
        // Player-edit chunks skip terrain gen; mark Ready so meshing picks
        // them up on the next updateChunkMeshes pass.
        chunk->setState(ChunkState::Ready);
    }

    // Set the block in the chunk
    chunk->setBlock(localPos.x, localPos.y, localPos.z, type);

    // If the modified block lies on a chunk face, the neighboring chunk's mesh
    // depends on it (visibility / AO). A single block can touch up to three
    // neighbors (corner). Mark each affected neighbor.
    struct Boundary {
        int axis;
        bool atEdge;
        glm::ivec3 offset;
    };
    const Boundary boundaries[6] = {
        {localPos.x, localPos.x == 0, {-1, 0, 0}},
        {localPos.x, localPos.x == CHUNK_SIZE - 1, {1, 0, 0}},
        {localPos.y, localPos.y == 0, {0, -1, 0}},
        {localPos.y, localPos.y == CHUNK_SIZE - 1, {0, 1, 0}},
        {localPos.z, localPos.z == 0, {0, 0, -1}},
        {localPos.z, localPos.z == CHUNK_SIZE - 1, {0, 0, 1}},
    };
    for (const auto& b : boundaries) {
        if (!b.atEdge)
            continue;
        if (Chunk* neighbor = getChunk(chunkPos + b.offset)) {
            neighbor->markModified();
        }
    }
    // updateChunkMeshes runs from the main loop; marked chunks will re-mesh then.
}

float VoxelWorld::getHeightAt(float x, float z) const {
    // Fast path: sample the terrain generator's noise directly instead of
    // scanning blocks top-down (which used to be ~40 getBlockAt calls per
    // frame from Player::updatePosition).
    //
    // Match generateTerrain's placement rule: terrainVoxels = (int)height,
    // so the topmost solid voxel is at y = terrainVoxels - 1 and the top
    // face sits at terrainVoxels * cellSize in world space.
    //
    // This sees pure terrain only — player block edits are not reflected.
    // Acceptable today (no break/place gameplay yet); revisit when that
    // lands by falling back to a block scan for modified columns.
    const float voxelX = x / m_params.cellSize;
    const float voxelZ = z / m_params.cellSize;
    const float voxelHeight = m_terrainGenerator->sampleHeight(voxelX, voxelZ);
    const int terrainVoxels = static_cast<int>(voxelHeight);
    return terrainVoxels * m_params.cellSize;
}

bool VoxelWorld::checkCollision(const Player& player) const {
    const Vec3 playerPos = player.getPosition();
    const float playerSize = player.getCollisionRadius();
    const float stepSize = std::max(m_params.cellSize * 0.5f, 0.05f);
    // Iterate via integer counters scaled by stepSize — avoids float-as-loop-
    // counter precision drift (and the cert-flp30-c lint).
    const auto stepsAlong = [stepSize](float extent) { return static_cast<int>(std::ceil(extent / stepSize)) + 1; };
    const int ySteps = stepsAlong(player.getHeight());
    const int xzSteps = stepsAlong(2.0f * playerSize);
    bool collision = false;
    for (int iy = 0; iy < ySteps; ++iy) {
        const float y = playerPos.y + static_cast<float>(iy) * stepSize;
        if (y > playerPos.y + player.getHeight())
            break;
        for (int iz = 0; iz < xzSteps; ++iz) {
            const float z = (playerPos.z - playerSize) + static_cast<float>(iz) * stepSize;
            if (z > playerPos.z + playerSize)
                break;
            for (int ix = 0; ix < xzSteps; ++ix) {
                const float x = (playerPos.x - playerSize) + static_cast<float>(ix) * stepSize;
                if (x > playerPos.x + playerSize)
                    break;
                const BlockType block = getBlockAt(glm::vec3(x, y, z));
                if (block != BlockType::AIR && BlockData::isSolid(block)) {
                    collision = true;
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

void VoxelWorld::renderCollisionDebug(const Camera& camera, const Player& /*player*/, float aspectRatio) {
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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)nullptr);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Lazy-init debug shader as a member (lifetime tied to VoxelWorld; no leak).
    if (!m_debugShader) {
        try {
            m_debugShader = std::make_unique<Shader>("assets/shaders/debug.vert", "assets/shaders/debug.frag");
        } catch (const std::exception& e) {
            Logger::logError("Failed to create debug shader: " + std::string(e.what()));
            return;
        }
    }

    // Update buffer with collision points
    glBindBuffer(GL_ARRAY_BUFFER, m_debugVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_collisionDebugPoints.size() * sizeof(glm::vec3),
                 m_collisionDebugPoints.data(),
                 GL_DYNAMIC_DRAW);

    // Render points
    m_debugShader->use();
    m_debugShader->setMat4("view", camera.getViewMatrix());
    m_debugShader->setMat4("projection", camera.getProjectionMatrix(aspectRatio));
    m_debugShader->setVec4("color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red points

    // Point size for visibility, configurable
    float const pointSize = Config::getFloat("Debug.collision_point_size", 5.0f);
    glPointSize(pointSize);

    // Disable depth test temporarily to make points visible through blocks
    GLboolean depthEnabled = 0;
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
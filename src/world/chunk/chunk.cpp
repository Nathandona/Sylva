#include "chunk.h"
#include "core/logger.h"
#include "core/config.h"
#include "renderer/shader.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Sylva {

namespace {
// Config-driven cell size, cached after first read. Config is loaded before any
// chunk is constructed, so the lazy-init via function-local static is safe.
float cachedCellSize() {
    static const float kCellSize = Config::getFloat("World.cell_size", 0.1f);
    return kCellSize;
}
} // namespace

// Chunk face vertices (for a unit cube at origin)
// Each face consists of 4 vertices (positions only for now)
// We'll add texture coordinates during mesh generation
const float FACE_VERTICES[6][12] = {
    // +X face
    {1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},
    // -X face
    {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    // +Y face
    {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
    // -Y face
    {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f},
    // +Z face
    {1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
    // -Z face
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f}};

// Texture coordinates for each face (same for all faces in this simplified version)
const float FACE_UVS[8] = {
    0.0f,
    0.0f, // Bottom-left
    0.0f,
    1.0f, // Top-left
    1.0f,
    1.0f, // Top-right
    1.0f,
    0.0f // Bottom-right
};

// Add normals for each face
const float FACE_NORMALS[6][3] = {
    {1.0f, 0.0f, 0.0f},  // +X face
    {-1.0f, 0.0f, 0.0f}, // -X face
    {0.0f, 1.0f, 0.0f},  // +Y face
    {0.0f, -1.0f, 0.0f}, // -Y face
    {0.0f, 0.0f, 1.0f},  // +Z face
    {0.0f, 0.0f, -1.0f}  // -Z face
};

Chunk::Chunk(const glm::ivec3& position) : m_position(position) {
    // Initialize all blocks to AIR
    m_blocks.fill(BlockType::AIR);

    Logger::logDebug("Chunk created at position (" + std::to_string(position.x) + ", " + std::to_string(position.y) +
                     ", " + std::to_string(position.z) + ")");
}

Chunk::~Chunk() {
    cleanupGraphics();
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (!isInBounds(x, y, z)) {
        return BlockType::AIR; // Out of bounds blocks are treated as air
    }

    return m_blocks[localPosToIndex(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    if (!isInBounds(x, y, z)) {
        return; // Ignore out of bounds blocks
    }

    int const index = localPosToIndex(x, y, z);

    // Only update if the block type is changing
    if (m_blocks[index] != type) {
        BlockType const oldType = m_blocks[index];
        m_blocks[index] = type;
        m_isModified = true;

        // Update isEmpty flag
        if (m_isEmpty && type != BlockType::AIR) {
            m_isEmpty = false;
        } else if (!m_isEmpty && oldType != BlockType::AIR && type == BlockType::AIR) {
            // Check if the chunk is now empty
            m_isEmpty = true;
            for (const auto& block : m_blocks) {
                if (block != BlockType::AIR) {
                    m_isEmpty = false;
                    break;
                }
            }
        }
    }
}

float Chunk::calculateVertexAO(int vx_local, int vy_local, int vz_local, const BlockSampler& sampler) const {
    // The 8 voxels sharing this vertex are at offsets {-1, 0} on each axis.
    // Sampler resolves out-of-bounds queries (incl. diagonals) via the world.
    int solidNeighbors = 0;
    for (int dz = -1; dz <= 0; ++dz) {
        for (int dy = -1; dy <= 0; ++dy) {
            for (int dx = -1; dx <= 0; ++dx) {
                BlockType const b = sampler(vx_local + dx, vy_local + dy, vz_local + dz);
                if (b != BlockType::AIR && BlockData::isSolid(b)) {
                    ++solidNeighbors;
                }
            }
        }
    }
    return 1.0f - (static_cast<float>(solidNeighbors) / 8.0f);
}

void Chunk::initializeMeshBuffers(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    // Create OpenGL objects
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    // Bind VAO first
    glBindVertexArray(m_vao);

    // Set up vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Set up index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
}

void Chunk::generateVertexData(std::vector<float>& vertices, std::vector<unsigned int>& indices, const BlockSampler& sampler) {
    vertices.reserve(static_cast<size_t>(CHUNK_VOLUME) * 24);
    indices.reserve(static_cast<size_t>(CHUNK_VOLUME) * 36);

    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                BlockType const blockType = getBlock(x, y, z);
                if (blockType == BlockType::AIR)
                    continue;
                for (int face = 0; face < 6; face++) {
                    if (shouldRenderFace(x, y, z, face, sampler)) {
                        addFaceToMesh(vertices, indices, x, y, z, face, blockType, sampler);
                    }
                }
            }
        }
    }
}

void Chunk::generateIndexData(std::vector<unsigned int>& indices, unsigned int baseVertex, unsigned int /*vertexCount*/) {
    // Add indices for two triangles (6 indices total)
    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 1);
    indices.push_back(baseVertex + 2);
    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 2);
    indices.push_back(baseVertex + 3);
}

void Chunk::uploadMeshToGPU(const std::vector<float>& /*vertices*/, const std::vector<unsigned int>& indices) {
    // glVertexAttribPointer takes the byte offset as a const void* — the
    // canonical OpenGL idiom. tidy flags the int->pointer conversion, but
    // it's required by the GL ABI. Suppress per call.
    const GLsizei kStride = 12 * sizeof(float);
    auto byteOffset = [](size_t bytes) -> const void* {
        return reinterpret_cast<const void*>(bytes); // NOLINT(performance-no-int-to-ptr)
    };

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kStride, byteOffset(0)); // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kStride, byteOffset(3 * sizeof(float))); // color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, kStride, byteOffset(6 * sizeof(float))); // uv
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, kStride, byteOffset(8 * sizeof(float))); // normal
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, kStride, byteOffset(11 * sizeof(float))); // AO
    glEnableVertexAttribArray(4);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Store index count for rendering
    m_indexCount = static_cast<unsigned int>(indices.size());
}

void Chunk::generateMesh(const BlockSampler& sampler) {
    if (!m_isModified && m_hasMesh) {
        return; // Skip if the mesh is up to date
    }

    cleanupGraphics();

    if (m_isEmpty) {
        m_isModified = false;
        return;
    }

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateVertexData(vertices, indices, sampler);

    // If no faces to render, mark as not modified and return
    if (indices.empty()) {
        m_isModified = false;
        return;
    }

    // Initialize and upload mesh data
    initializeMeshBuffers(vertices, indices);
    uploadMeshToGPU(vertices, indices);

    // Mark as up to date
    m_isModified = false;
    m_hasMesh = true;

    Logger::logDebug("Chunk mesh generated with " + std::to_string(vertices.size() / 12) + " vertices and " +
                     std::to_string(m_indexCount) + " indices");
}

void Chunk::buildMeshCpu(const BlockSampler& sampler) {
    // Pure CPU — no GL calls. Safe to run on a worker thread.
    // m_isEmpty is set during terrain gen (also worker), already released by
    // the state flip to Ready before this runs.
    auto data = std::make_unique<MeshCpuData>();
    if (!m_isEmpty) {
        generateVertexData(data->vertices, data->indices, sampler);
    }
    m_pendingMesh = std::move(data);
}

bool Chunk::uploadPendingMesh() {
    // Main thread only — touches GL.
    if (!m_pendingMesh) {
        return false;
    }

    cleanupGraphics();

    const bool hasGeometry = !m_pendingMesh->indices.empty();
    if (hasGeometry) {
        initializeMeshBuffers(m_pendingMesh->vertices, m_pendingMesh->indices);
        uploadMeshToGPU(m_pendingMesh->vertices, m_pendingMesh->indices);
        m_hasMesh = true;
    } else {
        // Empty chunk (all AIR or fully occluded). Still consume the pending
        // data; render() skips it via m_hasMesh / m_isEmpty.
        m_hasMesh = false;
    }
    m_isModified = false;
    m_pendingMesh.reset();
    return true;
}

void Chunk::render(Shader* shader) {
    if (!m_hasMesh || m_isEmpty) {
        return;
    }

    // Only model matrix changes per chunk — view/projection/viewPos are set
    // once per frame by VoxelWorld::render before this loop runs.
    auto modelMatrix = glm::mat4(1.0f);
    glm::vec3 const worldPos = chunkToWorldPos(m_position);
    modelMatrix = glm::translate(modelMatrix, worldPos);
    const float cellSize = cachedCellSize();
    modelMatrix = glm::scale(modelMatrix, glm::vec3(cellSize, cellSize, cellSize));
    shader->setMat4("model", modelMatrix);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

glm::vec3 Chunk::chunkToWorldPos(const glm::ivec3& chunkPos) {
    // Get cell size from config or use default
    const float cellSize = cachedCellSize();

    return glm::vec3(chunkPos.x * CHUNK_SIZE * cellSize,
                     chunkPos.y * CHUNK_SIZE * cellSize,
                     chunkPos.z * CHUNK_SIZE * cellSize);
}

glm::ivec3 Chunk::worldToChunkPos(const glm::vec3& worldPos) {
    // Get cell size from config or use default
    const float cellSize = cachedCellSize();

    return glm::ivec3(static_cast<int>(floor(worldPos.x / (CHUNK_SIZE * cellSize))),
                      static_cast<int>(floor(worldPos.y / (CHUNK_SIZE * cellSize))),
                      static_cast<int>(floor(worldPos.z / (CHUNK_SIZE * cellSize))));
}

glm::ivec3 Chunk::worldToLocalPos(const glm::vec3& worldPos) {
    // Get cell size from config or use default
    const float cellSize = cachedCellSize();

    glm::ivec3 const chunkPos = worldToChunkPos(worldPos);
    return glm::ivec3(static_cast<int>(floor(worldPos.x / cellSize)) - chunkPos.x * CHUNK_SIZE,
                      static_cast<int>(floor(worldPos.y / cellSize)) - chunkPos.y * CHUNK_SIZE,
                      static_cast<int>(floor(worldPos.z / cellSize)) - chunkPos.z * CHUNK_SIZE);
}

int Chunk::localPosToIndex(int x, int y, int z) const {
    return (y * CHUNK_SIZE * CHUNK_SIZE) + (z * CHUNK_SIZE) + x;
}

bool Chunk::shouldRenderFace(int x, int y, int z, int direction, const BlockSampler& sampler) const {
    static constexpr int OFFSETS[6][3] = {
        {1, 0, 0},  // +X
        {-1, 0, 0}, // -X
        {0, 1, 0},  // +Y
        {0, -1, 0}, // -Y
        {0, 0, 1},  // +Z
        {0, 0, -1}  // -Z
    };
    const int nx = x + OFFSETS[direction][0];
    const int ny = y + OFFSETS[direction][1];
    const int nz = z + OFFSETS[direction][2];
    return BlockData::isTransparent(sampler(nx, ny, nz));
}

glm::vec3 Chunk::calculateVertexPositions(int x, int y, int z, int direction, int i) const {
    float const vx = x + FACE_VERTICES[direction][i * 3 + 0];
    float const vy = y + FACE_VERTICES[direction][i * 3 + 1];
    float const vz = z + FACE_VERTICES[direction][i * 3 + 2];
    return glm::vec3(vx, vy, vz);
}

glm::vec3 Chunk::calculateVertexColors(BlockType blockType, int vx_local, int vy_local, int vz_local) const {
    glm::ivec3 const world_vertex_pos = m_position * CHUNK_SIZE + glm::ivec3(vx_local, vy_local, vz_local);
    return BlockData::getBlockColor(blockType, world_vertex_pos.x, world_vertex_pos.y, world_vertex_pos.z);
}

glm::vec3 Chunk::calculateVertexNormals(int direction) const {
    return glm::vec3(FACE_NORMALS[direction][0], FACE_NORMALS[direction][1], FACE_NORMALS[direction][2]);
}

void Chunk::addFaceToMesh(std::vector<float>& vertices,
                          std::vector<unsigned int>& indices,
                          int x,
                          int y,
                          int z,
                          int direction,
                          BlockType blockType,
                          const BlockSampler& sampler) {
    // Base index for the new vertices
    auto const baseIndex = static_cast<unsigned int>(vertices.size() / 12); // 12 floats per vertex

    // Get face normal
    glm::vec3 const normal = calculateVertexNormals(direction);

    // Add 4 vertices for this face
    for (int i = 0; i < 4; ++i) {
        // Calculate vertex position
        glm::vec3 const position = calculateVertexPositions(x, y, z, direction, i);

        // Calculate local vertex coordinates for AO and color
        int const vx_local = x + static_cast<int>(std::round(FACE_VERTICES[direction][i * 3 + 0]));
        int const vy_local = y + static_cast<int>(std::round(FACE_VERTICES[direction][i * 3 + 1]));
        int const vz_local = z + static_cast<int>(std::round(FACE_VERTICES[direction][i * 3 + 2]));

        // Calculate vertex color
        glm::vec3 const color = calculateVertexColors(blockType, vx_local, vy_local, vz_local);

        // Add vertex position
        vertices.push_back(position.x);
        vertices.push_back(position.y);
        vertices.push_back(position.z);

        // Add vertex color
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);

        // Add texture coordinates
        vertices.push_back(FACE_UVS[i * 2 + 0]);
        vertices.push_back(FACE_UVS[i * 2 + 1]);

        // Add normal
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);
        vertices.push_back(normal.z);

        // Add ambient occlusion
        float const aoFactor = calculateVertexAO(vx_local, vy_local, vz_local, sampler);
        vertices.push_back(aoFactor);
    }

    // Add indices for the face
    generateIndexData(indices, baseIndex, 4);
}

void Chunk::cleanupGraphics() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }

    m_indexCount = 0;
    m_hasMesh = false;
}

bool Chunk::isInBounds(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE;
}

} // namespace Sylva
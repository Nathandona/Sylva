#include "chunk.h"
#include "core/logger.h"
#include "core/config.h"
#include "renderer/shader.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Sylva {

// Chunk face vertices (for a unit cube at origin)
// Each face consists of 4 vertices (positions only for now)
// We'll add texture coordinates during mesh generation
const float FACE_VERTICES[6][12] = {
    // +X face
    { 1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f },
    // -X face
    { 0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f },
    // +Y face
    { 0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 0.0f },
    // -Y face
    { 0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 1.0f },
    // +Z face
    { 1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f },
    // -Z face
    { 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f }
};

// Texture coordinates for each face (same for all faces in this simplified version)
const float FACE_UVS[8] = {
    0.0f, 0.0f,  // Bottom-left
    0.0f, 1.0f,  // Top-left
    1.0f, 1.0f,  // Top-right
    1.0f, 0.0f   // Bottom-right
};

// Indices for a single face (two triangles)
const unsigned int FACE_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

// Add normals for each face
const float FACE_NORMALS[6][3] = {
    { 1.0f,  0.0f,  0.0f}, // +X face
    {-1.0f,  0.0f,  0.0f}, // -X face
    { 0.0f,  1.0f,  0.0f}, // +Y face
    { 0.0f, -1.0f,  0.0f}, // -Y face
    { 0.0f,  0.0f,  1.0f}, // +Z face
    { 0.0f,  0.0f, -1.0f}  // -Z face
};

// The calculateVertexAO function is now a static member of Chunk.
// Its definition will be moved below, within the Chunk class scope if not already picked up by the tool.
// For now, assuming the tool will handle moving the definition if it sees the declaration in the header.

Chunk::Chunk(const glm::ivec3& position)
    : m_position(position),
      m_vao(0),
      m_vbo(0),
      m_ebo(0),
      m_indexCount(0),
      m_isEmpty(true),
      m_isModified(true),
      m_hasMesh(false) {
    
    // Initialize all blocks to AIR
    m_blocks.fill(BlockType::AIR);
    
    Logger::logDebug("Chunk created at position (" + 
                    std::to_string(position.x) + ", " +
                    std::to_string(position.y) + ", " +
                    std::to_string(position.z) + ")");
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
    
    int index = localPosToIndex(x, y, z);
    
    // Only update if the block type is changing
    if (m_blocks[index] != type) {
        BlockType oldType = m_blocks[index];
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

// Definition of the static method
float Chunk::calculateVertexAO(const Chunk* chunk, int vpX, int vpY, int vpZ, const Chunk* surroundingChunks[6]) {
    int solidNeighbors = 0;
    // int totalChecks = 0; // Not strictly needed if we assume 8 checks unless bounded

    for (int dz_offset = -1; dz_offset <= 0; ++dz_offset) {
        for (int dy_offset = -1; dy_offset <= 0; ++dy_offset) {
            for (int dx_offset = -1; dx_offset <= 0; ++dx_offset) {
                int neighborX = vpX + dx_offset;
                int neighborY = vpY + dy_offset;
                int neighborZ = vpZ + dz_offset;
                BlockType neighborBlockType = BlockType::AIR;

                // Determine which chunk to check based on voxel coordinates relative to current chunk origin (vpX,vpY,vpZ)
                // The 'chunk' parameter to calculateVertexAO is the chunk *owning the vertex* for which AO is being calculated.
                // However, the voxels contributing to AO can be in this chunk or neighbors.
                // The logic needs to correctly determine if (neighborX, neighborY, neighborZ) falls into 'chunk' or one of 'surroundingChunks'.

                const Chunk* queryChunk = chunk; // Start by assuming the voxel is in the current chunk.
                int queryX = neighborX;
                int queryY = neighborY;
                int queryZ = neighborZ;

                if (neighborX < 0) {
                    queryChunk = surroundingChunks[1]; // -X neighbor
                    if(queryChunk) queryX = CHUNK_SIZE + neighborX;
                } else if (neighborX >= CHUNK_SIZE) {
                    queryChunk = surroundingChunks[0]; // +X neighbor
                    if(queryChunk) queryX = neighborX - CHUNK_SIZE;
                }
                // After potential X adjustment, check Y
                if (neighborY < 0) {
                    // If already shifted to a neighbor by X, use that neighbor's context for Y check
                    // This is tricky. Simplest is to re-evaluate based on original relative position from vertex.
                    // For now, let original surroundingChunks logic handle this by passing the original chunk the vertex belongs to.
                    // The original free function passed `chunk` which was the chunk being meshed.
                    // The vertex (vpX,vpY,vpZ) is on the boundary of that chunk.
                    // So, if vpX is 0, then neighborX = -1 IS in the -X chunk.

                    // Re-evaluating logic for clarity:
                    // (neighborX, neighborY, neighborZ) are local to the *chunk being meshed*.
                    // If they go out of bounds, we use surroundingChunks.

                    queryChunk = chunk; // Reset to current chunk for this check stream
                    queryX = neighborX; queryY = neighborY; queryZ = neighborZ; // Reset local coords too

                    if (neighborX < 0) { if (surroundingChunks[1]) { queryChunk = surroundingChunks[1]; queryX = CHUNK_SIZE + neighborX; } else { queryChunk = nullptr; } }
                    else if (neighborX >= CHUNK_SIZE) { if (surroundingChunks[0]) { queryChunk = surroundingChunks[0]; queryX = neighborX - CHUNK_SIZE; } else { queryChunk = nullptr; } }
                    // Now check Y based on potentially updated queryChunk and queryX
                    if (queryChunk && neighborY < 0) { if (surroundingChunks[3]) { queryChunk = surroundingChunks[3]; queryY = CHUNK_SIZE + neighborY; /* queryX might need to be re-relativized if original queryChunk changed */ } else { queryChunk = nullptr; } }
                    else if (queryChunk && neighborY >= CHUNK_SIZE) { if (surroundingChunks[2]) { queryChunk = surroundingChunks[2]; queryY = neighborY - CHUNK_SIZE; } else { queryChunk = nullptr; } }
                    // And Z
                    if (queryChunk && neighborZ < 0) { if (surroundingChunks[5]) { queryChunk = surroundingChunks[5]; queryZ = CHUNK_SIZE + neighborZ; } else { queryChunk = nullptr; } }
                    else if (queryChunk && neighborZ >= CHUNK_SIZE) { if (surroundingChunks[4]) { queryChunk = surroundingChunks[4]; queryZ = neighborZ - CHUNK_SIZE; } else { queryChunk = nullptr; } }
                    
                    // Fallback to simpler direct indexing if not out of bounds for initial chunk
                    if (chunk->isInBounds(neighborX, neighborY, neighborZ)) {
                         queryChunk = chunk;
                         queryX = neighborX; queryY = neighborY; queryZ = neighborZ;
                    }

                } else if (chunk->isInBounds(neighborX, neighborY, neighborZ)) {
                    // voxel is within the current chunk, no need to change queryChunk
                } else { // Out of bounds, determine which neighbor
                    // This logic is complex for corners where it could be in one of 3 neighbors.
                    // The original free function had a simplified version. Let's try to make it more robust.
                    glm::ivec3 localVoxelPos(neighborX, neighborY, neighborZ);
                    glm::ivec3 targetChunkOffset(0,0,0);
                    
                    if(localVoxelPos.x < 0) targetChunkOffset.x = -1; else if (localVoxelPos.x >= CHUNK_SIZE) targetChunkOffset.x = 1;
                    if(localVoxelPos.y < 0) targetChunkOffset.y = -1; else if (localVoxelPos.y >= CHUNK_SIZE) targetChunkOffset.y = 1;
                    if(localVoxelPos.z < 0) targetChunkOffset.z = -1; else if (localVoxelPos.z >= CHUNK_SIZE) targetChunkOffset.z = 1;

                    queryChunk = nullptr; // Assume no chunk found unless we identify one
                    // This needs a mapping from offset to surroundingChunks index or direct use of VoxelWorld::getChunk
                    // The surroundingChunks array is indexed: +X [0], -X [1], +Y [2], -Y [3], +Z [4], -Z [5]
                    if(targetChunkOffset.x == 1 && surroundingChunks[0]) { queryChunk = surroundingChunks[0]; queryX = localVoxelPos.x - CHUNK_SIZE; }
                    else if(targetChunkOffset.x == -1 && surroundingChunks[1]) { queryChunk = surroundingChunks[1]; queryX = localVoxelPos.x + CHUNK_SIZE; }
                    // ... and so on for Y and Z, and combinations for corners. This is getting too complex for a simple port.
                    // The original code was simpler and probably had issues at chunk corners/edges for AO.
                    // For now, stick to the original free function's level of complexity for neighbor checking.
                    
                    // Reverting to simpler neighbor check from original free function for AO calculation:
                    queryChunk = chunk; // The chunk the vertex *belongs* to.
                    queryX = neighborX; queryY = neighborY; queryZ = neighborZ;

                    bool isInCurrentChunk = chunk->isInBounds(queryX, queryY, queryZ);
                    if (isInCurrentChunk) {
                        neighborBlockType = queryChunk->getBlock(queryX, queryY, queryZ);
                    } else { // Voxel for AO is outside the current chunk. Check neighbors.
                        if (queryX < 0 && surroundingChunks[1]) { neighborBlockType = surroundingChunks[1]->getBlock(CHUNK_SIZE + queryX, queryY, queryZ); }
                        else if (queryX >= CHUNK_SIZE && surroundingChunks[0]) { neighborBlockType = surroundingChunks[0]->getBlock(queryX - CHUNK_SIZE, queryY, queryZ); }
                        else if (queryY < 0 && surroundingChunks[3]) { neighborBlockType = surroundingChunks[3]->getBlock(queryX, CHUNK_SIZE + queryY, queryZ); }
                        else if (queryY >= CHUNK_SIZE && surroundingChunks[2]) { neighborBlockType = surroundingChunks[2]->getBlock(queryX, queryY - CHUNK_SIZE, queryZ); }
                        else if (queryZ < 0 && surroundingChunks[5]) { neighborBlockType = surroundingChunks[5]->getBlock(queryX, queryY, CHUNK_SIZE + queryZ); }
                        else if (queryZ >= CHUNK_SIZE && surroundingChunks[4]) { neighborBlockType = surroundingChunks[4]->getBlock(queryX, queryY, queryZ - CHUNK_SIZE); }
                        // This simplified check doesn't handle voxels that are diagonal to the current chunk for AO.
                        // e.g. if vpX=0, vpY=0, then neighborX=-1, neighborY=-1 needs to check the chunk at (-1,-1,0) relative.
                        // The surroundingChunks only gives direct face neighbors.
                        // True corner AO needs access to more neighbors or a VoxelWorld reference.
                        // For now, this will produce AO artifacts at chunk edges/corners.
                    }
                }

                if (queryChunk && queryChunk->isInBounds(queryX, queryY, queryZ)) {
                     neighborBlockType = queryChunk->getBlock(queryX, queryY, queryZ);
                } // else: it remains AIR if no valid chunk/coords found.

                if (neighborBlockType != BlockType::AIR && BlockData::isSolid(neighborBlockType)) {
                    solidNeighbors++;
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

void Chunk::generateVertexData(std::vector<float>& vertices, std::vector<unsigned int>& indices, const Chunk* surroundingChunks[6]) {
    // Reserve space to avoid reallocations
    vertices.reserve(CHUNK_VOLUME * 24); // Worst case: all blocks visible, 4 vertices per face, 6 faces
    indices.reserve(CHUNK_VOLUME * 36);  // Worst case: all blocks visible, 6 indices per face, 6 faces
    
    // For each block in the chunk
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                BlockType blockType = getBlock(x, y, z);
                
                // Skip air blocks
                if (blockType == BlockType::AIR) {
                    continue;
                }
                
                // Check each face
                for (int face = 0; face < 6; face++) {
                    if (shouldRenderFace(x, y, z, face, surroundingChunks)) {
                        addFaceToMesh(vertices, indices, x, y, z, face, blockType, surroundingChunks);
                    }
                }
            }
        }
    }
}

void Chunk::generateIndexData(std::vector<unsigned int>& indices, unsigned int baseVertex, unsigned int vertexCount) {
    // Add indices for two triangles (6 indices total)
    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 1);
    indices.push_back(baseVertex + 2);
    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 2);
    indices.push_back(baseVertex + 3);
}

void Chunk::uploadMeshToGPU(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    // Position attribute (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinates attribute (2 floats)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Normal attribute (3 floats)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    
    // AO attribute (1 float)
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Store index count for rendering
    m_indexCount = static_cast<unsigned int>(indices.size());
}

void Chunk::generateMesh(const Chunk* surroundingChunks[6]) {
    if (!m_isModified && m_hasMesh) {
        return; // Skip if the mesh is up to date
    }
    
    // Clean up old mesh if it exists
    cleanupGraphics();
    
    // Skip empty chunks (just set as not modified)
    if (m_isEmpty) {
        m_isModified = false;
        return;
    }
    
    // Generate mesh data
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateVertexData(vertices, indices, surroundingChunks);
    
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
    
    Logger::logDebug("Chunk mesh generated with " + std::to_string(vertices.size() / 12) + 
                    " vertices and " + std::to_string(m_indexCount) + " indices");
}

void Chunk::render(Shader* shader, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    // Skip if no mesh or empty
    if (!m_hasMesh || m_isEmpty) {
        return;
    }
    
    // Use shader
    shader->use();
    
    // Set matrices
    glm::mat4 modelMatrix = glm::mat4(1.0f); // Identity matrix
    
    // Translate to chunk world position
    glm::vec3 worldPos = chunkToWorldPos(m_position);
    modelMatrix = glm::translate(modelMatrix, worldPos);
    
    // Scale the chunk's local coordinates by cellSize
    float cellSize = Config::getFloat("World.cell_size", 0.1f);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(cellSize, cellSize, cellSize));
    
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    
    // Add camera position for enhanced lighting (view dependent effects)
    glm::mat4 invView = glm::inverse(viewMatrix);
    glm::vec3 cameraPos = glm::vec3(invView[3]);
    shader->setVec3("viewPos", cameraPos);
    
    // Draw chunk
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

glm::vec3 Chunk::chunkToWorldPos(const glm::ivec3& chunkPos) {
    // Get cell size from config or use default
    float cellSize = Config::getFloat("World.cell_size", 0.1f);
    
    return glm::vec3(
        chunkPos.x * CHUNK_SIZE * cellSize,
        chunkPos.y * CHUNK_SIZE * cellSize,
        chunkPos.z * CHUNK_SIZE * cellSize
    );
}

glm::ivec3 Chunk::worldToChunkPos(const glm::vec3& worldPos) {
    // Get cell size from config or use default
    float cellSize = Config::getFloat("World.cell_size", 0.1f);
    
    return glm::ivec3(
        static_cast<int>(floor(worldPos.x / (CHUNK_SIZE * cellSize))),
        static_cast<int>(floor(worldPos.y / (CHUNK_SIZE * cellSize))),
        static_cast<int>(floor(worldPos.z / (CHUNK_SIZE * cellSize)))
    );
}

glm::ivec3 Chunk::worldToLocalPos(const glm::vec3& worldPos) {
    // Get cell size from config or use default
    float cellSize = Config::getFloat("World.cell_size", 0.1f);
    
    glm::ivec3 chunkPos = worldToChunkPos(worldPos);
    return glm::ivec3(
        static_cast<int>(floor(worldPos.x / cellSize)) - chunkPos.x * CHUNK_SIZE,
        static_cast<int>(floor(worldPos.y / cellSize)) - chunkPos.y * CHUNK_SIZE,
        static_cast<int>(floor(worldPos.z / cellSize)) - chunkPos.z * CHUNK_SIZE
    );
}

int Chunk::localPosToIndex(int x, int y, int z) const {
    return (y * CHUNK_SIZE * CHUNK_SIZE) + (z * CHUNK_SIZE) + x;
}

bool Chunk::shouldRenderFace(int x, int y, int z, int direction, const Chunk* surroundingChunks[6]) const {
    // Define direction offsets (+X, -X, +Y, -Y, +Z, -Z)
    const int OFFSETS[6][3] = {
        { 1,  0,  0}, // +X
        {-1,  0,  0}, // -X
        { 0,  1,  0}, // +Y
        { 0, -1,  0}, // -Y
        { 0,  0,  1}, // +Z
        { 0,  0, -1}  // -Z
    };
    
    int nx = x + OFFSETS[direction][0];
    int ny = y + OFFSETS[direction][1];
    int nz = z + OFFSETS[direction][2];
    
    // Check if the neighboring block is within this chunk
    if (isInBounds(nx, ny, nz)) {
        // If the neighboring block is transparent, we should render this face
        return BlockData::isTransparent(m_blocks[localPosToIndex(nx, ny, nz)]);
    }
    
    // If the neighboring block is in another chunk, check the appropriate surrounding chunk
    if (surroundingChunks[direction] != nullptr) {
        // Convert to local coordinates in the neighboring chunk
        int localX = nx;
        int localY = ny;
        int localZ = nz;
        
        // Adjust coordinates based on the direction
        if (direction == 0) localX = 0;         // +X face -> check -X face of next chunk
        if (direction == 1) localX = CHUNK_SIZE - 1; // -X face -> check +X face of prev chunk
        if (direction == 2) localY = 0;         // +Y face -> check -Y face of next chunk
        if (direction == 3) localY = CHUNK_SIZE - 1; // -Y face -> check +Y face of prev chunk
        if (direction == 4) localZ = 0;         // +Z face -> check -Z face of next chunk
        if (direction == 5) localZ = CHUNK_SIZE - 1; // -Z face -> check +Z face of prev chunk
        
        // Return true if the neighboring block in the other chunk is transparent
        return BlockData::isTransparent(surroundingChunks[direction]->getBlock(localX, localY, localZ));
    }
    
    // If there's no neighboring chunk, assume air (transparent)
    return true;
}

glm::vec3 Chunk::calculateVertexPositions(int x, int y, int z, int direction, int i) const {
    float vx = x + FACE_VERTICES[direction][i * 3 + 0];
    float vy = y + FACE_VERTICES[direction][i * 3 + 1];
    float vz = z + FACE_VERTICES[direction][i * 3 + 2];
    return glm::vec3(vx, vy, vz);
}

glm::vec3 Chunk::calculateVertexColors(BlockType blockType, int vx_local, int vy_local, int vz_local) const {
    glm::ivec3 world_vertex_pos = m_position * CHUNK_SIZE + glm::ivec3(vx_local, vy_local, vz_local);
    return BlockData::getBlockColor(blockType, world_vertex_pos.x, world_vertex_pos.y, world_vertex_pos.z);
}

glm::vec3 Chunk::calculateVertexNormals(int direction) const {
    return glm::vec3(
        FACE_NORMALS[direction][0],
        FACE_NORMALS[direction][1],
        FACE_NORMALS[direction][2]
    );
}

float Chunk::calculateVertexAO(int vx_local, int vy_local, int vz_local, const Chunk* surroundingChunks[6]) const {
    return calculateVertexAO(this, vx_local, vy_local, vz_local, surroundingChunks);
}

void Chunk::addFaceToMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices, 
                         int x, int y, int z, int direction, BlockType blockType,
                         const Chunk* surroundingChunks[6]) {
    // Base index for the new vertices
    unsigned int baseIndex = static_cast<unsigned int>(vertices.size() / 12); // 12 floats per vertex

    // Get face normal
    glm::vec3 normal = calculateVertexNormals(direction);

    // Add 4 vertices for this face
    for (int i = 0; i < 4; ++i) {
        // Calculate vertex position
        glm::vec3 position = calculateVertexPositions(x, y, z, direction, i);
        
        // Calculate local vertex coordinates for AO and color
        int vx_local = x + static_cast<int>(std::round(FACE_VERTICES[direction][i * 3 + 0]));
        int vy_local = y + static_cast<int>(std::round(FACE_VERTICES[direction][i * 3 + 1]));
        int vz_local = z + static_cast<int>(std::round(FACE_VERTICES[direction][i * 3 + 2]));

        // Calculate vertex color
        glm::vec3 color = calculateVertexColors(blockType, vx_local, vy_local, vz_local);

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
        float aoFactor = calculateVertexAO(vx_local, vy_local, vz_local, surroundingChunks);
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
    return x >= 0 && x < CHUNK_SIZE && 
           y >= 0 && y < CHUNK_SIZE && 
           z >= 0 && z < CHUNK_SIZE;
}

} // namespace Sylva 
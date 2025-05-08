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

// (Helper function to be placed preferably in an accessible scope or as a static member if appropriate)
// For simplicity here, we define it before its first use or assume it's declared in the header if it were a member.
// This simple version calculates AO based on 8 neighbors of a vertex point.
float calculateVertexAO(const Chunk* chunk, int vpX, int vpY, int vpZ, const Chunk* surroundingChunks[6]) {
    int solidNeighbors = 0;
    int totalChecks = 0; // Should be 8

    // Check the 8 voxels that meet at the vertex point (vpX, vpY, vpZ)
    // These are voxels whose corner *is* this vertex point.
    // Example: vertex (5,5,5) is a corner for voxels (4,4,4) through (5,5,5) in local chunk coords
    for (int dz_offset = -1; dz_offset <= 0; ++dz_offset) {
        for (int dy_offset = -1; dy_offset <= 0; ++dy_offset) {
            for (int dx_offset = -1; dx_offset <= 0; ++dx_offset) {
                int neighborX = vpX + dx_offset;
                int neighborY = vpY + dy_offset;
                int neighborZ = vpZ + dz_offset;
                BlockType neighborBlockType = BlockType::AIR;

                if (chunk->isInBounds(neighborX, neighborY, neighborZ)) {
                    neighborBlockType = chunk->getBlock(neighborX, neighborY, neighborZ);
                } else {
                    // Vertex is at the edge of the chunk, need to check surrounding chunks
                    glm::ivec3 worldVoxelPos = chunk->getPosition() * CHUNK_SIZE + glm::ivec3(neighborX, neighborY, neighborZ);
                    
                    // Determine which neighbor chunk to check based on out-of-bounds coordinates
                    // This logic is simplified and might need refinement for all corner/edge cases
                    if (neighborX < 0 && surroundingChunks[1]) { // -X neighbor
                        neighborBlockType = surroundingChunks[1]->getBlock(CHUNK_SIZE + neighborX, neighborY, neighborZ);
                    } else if (neighborX >= CHUNK_SIZE && surroundingChunks[0]) { // +X neighbor
                        neighborBlockType = surroundingChunks[0]->getBlock(neighborX - CHUNK_SIZE, neighborY, neighborZ);
                    } else if (neighborY < 0 && surroundingChunks[3]) { // -Y neighbor
                         neighborBlockType = surroundingChunks[3]->getBlock(neighborX, CHUNK_SIZE + neighborY, neighborZ);
                    } else if (neighborY >= CHUNK_SIZE && surroundingChunks[2]) { // +Y neighbor
                        neighborBlockType = surroundingChunks[2]->getBlock(neighborX, neighborY - CHUNK_SIZE, neighborZ);
                    } else if (neighborZ < 0 && surroundingChunks[5]) { // -Z neighbor
                        neighborBlockType = surroundingChunks[5]->getBlock(neighborX, neighborY, CHUNK_SIZE + neighborZ);
                    } else if (neighborZ >= CHUNK_SIZE && surroundingChunks[4]) { // +Z neighbor
                        neighborBlockType = surroundingChunks[4]->getBlock(neighborX, neighborY, neighborZ - CHUNK_SIZE);
                    }
                    // Diagonal/corner out-of-bounds checks are more complex and omitted for brevity here
                    // A more robust solution would convert to world coordinates and check the appropriate neighbor.
                }
                
                if (neighborBlockType != BlockType::AIR && BlockData::isSolid(neighborBlockType)) {
                    solidNeighbors++;
                }
                totalChecks++;
            }
        }
    }
    // If totalChecks is not 8, it means some neighbors were outside the current chunk AND no corresponding surrounding chunk was provided/checked properly.
    // This can happen at world edges or if neighbor chunks aren't loaded.
    // A simple approach: if we couldn't check all 8, assume less occlusion.
    // A more robust approach would ensure all 8 are checked using surroundingChunks.

    return 1.0f - (static_cast<float>(solidNeighbors) / 8.0f); // Normalize by 8 potential occluders
}

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
    
    // Prepare vertex and index buffers
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Reserve some space to avoid frequent reallocations
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
                        // Pass surroundingChunks to addFaceToMesh
                        addFaceToMesh(vertices, indices, x, y, z, face, blockType, surroundingChunks);
                    }
                }
            }
        }
    }
    
    // If no faces to render, mark as not modified and return
    if (indices.empty()) {
        m_isModified = false;
        return;
    }
    
    // Create OpenGL objects
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
    
    // Bind and upload data
    glBindVertexArray(m_vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinates attribute (2 floats)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Normal attribute (3 floats) - new
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    
    // AO attribute (1 float) - new
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Store index count for rendering
    m_indexCount = static_cast<unsigned int>(indices.size());
    
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

void Chunk::addFaceToMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices, 
                         int x, int y, int z, int direction, BlockType blockType,
                         const Chunk* surroundingChunks[6]) {
    // Base index for the new vertices
    unsigned int baseIndex = static_cast<unsigned int>(vertices.size() / 12); // 12 floats per vertex

    // Get face normal
    glm::vec3 normal(FACE_NORMALS[direction][0], FACE_NORMALS[direction][1], FACE_NORMALS[direction][2]);

    // Add 4 vertices for this face
    for (int i = 0; i < 4; ++i) {
        // Vertex position relative to block origin (0,0,0)
        float vx_rel = FACE_VERTICES[direction][i * 3 + 0];
        float vy_rel = FACE_VERTICES[direction][i * 3 + 1];
        float vz_rel = FACE_VERTICES[direction][i * 3 + 2];

        // Absolute vertex position within the chunk (local voxel coordinates for the vertex itself)
        // Example: for a block at (x,y,z), a vertex of its +X face could be at (x+1, y, z)
        int vx_local = x + static_cast<int>(vx_rel); // Potential issue if vx_rel is not exactly 0 or 1
        int vy_local = y + static_cast<int>(vy_rel);
        int vz_local = z + static_cast<int>(vz_rel);
        
        // For robust vertex coordinate, consider the face direction.
        // A vertex is shared by 3 faces. Its coordinates are integer points in the grid.
        // E.g., for +X face of block (x,y,z), vertices are (x+1,y,z), (x+1,y+1,z), (x+1,y+1,z+1), (x+1,y,z+1)
        // We need the exact integer grid point for this vertex.

        // Corrected local vertex coordinates (integer grid points)
        // These are the coordinates of the corner point this vertex represents.
        int corner_vx = x + (normal.x > 0 ? 1 : (vx_rel > 0.5f ? 1 : 0));
        if (normal.x < 0) corner_vx = x + (vx_rel > 0.5f ? 1 : 0); // For -X face, vertices are at x or x+1 plane. FACE_VERTICES already define them at x=0 plane.

        int corner_vy = y + (normal.y > 0 ? 1 : (vy_rel > 0.5f ? 1 : 0));
        if (normal.y < 0) corner_vy = y + (vy_rel > 0.5f ? 1 : 0);

        int corner_vz = z + (normal.z > 0 ? 1 : (vz_rel > 0.5f ? 1 : 0));
        if (normal.z < 0) corner_vz = z + (vz_rel > 0.5f ? 1 : 0);


        // More direct way to get corner_v* from FACE_VERTICES which are 0 or 1
        // For the k-th vertex of the face of block (x,y,z)
        int actual_corner_vx = x + static_cast<int>(FACE_VERTICES[direction][i*3+0] + (FACE_NORMALS[direction][0] < 0 ? 1.0f : 0.0f) );
        // This is not quite right either. FACE_VERTICES define unit cube.
        // Vertices of a face are at corners of the block x,y,z.
        // Example for +X face: (x+1,y,z), (x+1,y+1,z), (x+1,y+1,z+1), (x+1,y,z+1)

        // Let's define the four corner points for the current vertex i of the face:
        // The FACE_VERTICES are defined for a unit cube at origin.
        // If block is at (x,y,z), its actual vertex positions are (x + fv.x, y + fv.y, z + fv.z)
        // These are the vertex positions that should be used for AO and color variation.
        vx_local = x + static_cast<int>(std::round(FACE_VERTICES[direction][i * 3 + 0]));
        vy_local = y + static_cast<int>(std::round(FACE_VERTICES[direction][i * 3 + 1]));
        vz_local = z + static_cast<int>(std::round(FACE_VERTICES[direction][i * 3 + 2]));


        // Vertex position (floats, for rendering)
        vertices.push_back(x + FACE_VERTICES[direction][i * 3 + 0]); // Relative to chunk origin
        vertices.push_back(y + FACE_VERTICES[direction][i * 3 + 1]);
        vertices.push_back(z + FACE_VERTICES[direction][i * 3 + 2]);

        // Vertex color (with per-vertex hue variation)
        // Calculate world integer coordinates for this vertex
        glm::ivec3 world_vertex_pos = m_position * CHUNK_SIZE + glm::ivec3(vx_local, vy_local, vz_local);
        glm::vec3 color = BlockData::getBlockColor(blockType, world_vertex_pos.x, world_vertex_pos.y, world_vertex_pos.z);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);

        // Texture coordinates
        vertices.push_back(FACE_UVS[i * 2 + 0]);
        vertices.push_back(FACE_UVS[i * 2 + 1]);

        // Normal
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);
        vertices.push_back(normal.z);
        
        // Ambient Occlusion factor (per-vertex)
        // Call improved calculateVertexAO with surrounding chunks
        float aoFactor = calculateVertexAO(this, vx_local, vy_local, vz_local, surroundingChunks);
        vertices.push_back(aoFactor);
    }

    // Add indices for the face
    for (int i = 0; i < 6; i++) {
        indices.push_back(baseIndex + FACE_INDICES[i]);
    }
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
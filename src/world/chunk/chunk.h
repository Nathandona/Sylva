#pragma once

#include "../block.h"
#include <array>
#include <vector>
#include <functional>
#include <glm/glm.hpp>

namespace Sylva {

// Forward declarations
class Shader;

/**
 * @brief Block sampler callback used during meshing.
 *
 * Coordinates are chunk-local for the chunk being meshed and may go outside
 * [0, CHUNK_SIZE). The sampler resolves neighboring chunks (including diagonals)
 * via the owning world. Out-of-world queries return BlockType::AIR.
 */
using BlockSampler = std::function<BlockType(int x, int y, int z)>;

/**
 * @brief Size constants for chunks
 */
constexpr int CHUNK_SIZE = 32; // Size of a chunk in blocks (32x32x32) - increased for micro-voxel density
constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

/**
 * @brief A 3D grid of micro-blocks that makes up a section of the world
 */
class Chunk {
public:
    /**
     * @brief Constructor
     * @param position The position of the chunk in chunk coordinates
     */
    Chunk(const glm::ivec3& position);

    /**
     * @brief Destructor
     */
    ~Chunk();

    /**
     * @brief Get the position of this chunk in chunk coordinates
     * @return The chunk's position
     */
    const glm::ivec3& getPosition() const { return m_position; }

    /**
     * @brief Get the block type at the specified local position
     * @param x Local X coordinate (0 to CHUNK_SIZE-1)
     * @param y Local Y coordinate (0 to CHUNK_SIZE-1)
     * @param z Local Z coordinate (0 to CHUNK_SIZE-1)
     * @return The block type at the position
     */
    BlockType getBlock(int x, int y, int z) const;

    /**
     * @brief Set the block type at the specified local position
     * @param x Local X coordinate (0 to CHUNK_SIZE-1)
     * @param y Local Y coordinate (0 to CHUNK_SIZE-1)
     * @param z Local Z coordinate (0 to CHUNK_SIZE-1)
     * @param type The new block type
     */
    void setBlock(int x, int y, int z, BlockType type);

    /**
     * @brief Check if the chunk is empty (contains only air blocks)
     * @return True if the chunk is empty, false otherwise
     */
    bool isEmpty() const { return m_isEmpty; }

    /**
     * @brief Generate the chunk mesh based on current blocks
     * @param sampler Callback returning the block at any chunk-local coordinate;
     *                resolves to neighbor chunks (including diagonals) for
     *                out-of-bounds queries.
     */
    void generateMesh(const BlockSampler& sampler);

    /**
     * @brief Render the chunk
     * @param shader The shader to use for rendering
     * @param viewMatrix The camera view matrix
     * @param projectionMatrix The camera projection matrix
     */
    void render(Shader* shader, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    /**
     * @brief Check if the chunk has been modified since last mesh generation
     * @return True if the chunk has been modified, false otherwise
     */
    bool isModified() const { return m_isModified; }

    /**
     * @brief Mark the chunk as modified, forcing a mesh rebuild on next update.
     */
    void markModified() { m_isModified = true; }

    /**
     * @brief Convert chunk coordinates to world coordinates
     * @param chunkPos The chunk position
     * @return The world coordinates of the chunk's origin
     */
    static glm::vec3 chunkToWorldPos(const glm::ivec3& chunkPos);

    /**
     * @brief Convert world coordinates to chunk coordinates
     * @param worldPos The world position
     * @return The chunk coordinates containing the world position
     */
    static glm::ivec3 worldToChunkPos(const glm::vec3& worldPos);

    /**
     * @brief Get local coordinates within a chunk from world coordinates
     * @param worldPos The world position
     * @return The local coordinates within the chunk (0 to CHUNK_SIZE-1)
     */
    static glm::ivec3 worldToLocalPos(const glm::vec3& worldPos);

    /**
     * @brief Check if the given position is inside the chunk
     * @param x Local X coordinate
     * @param y Local Y coordinate
     * @param z Local Z coordinate
     * @return True if the position is inside the chunk, false otherwise
     */
    bool isInBounds(int x, int y, int z) const;

private:
    /**
     * @brief Convert 3D local coordinates to a 1D array index
     * @param x Local X coordinate
     * @param y Local Y coordinate
     * @param z Local Z coordinate
     * @return The index in the blocks array
     */
    int localPosToIndex(int x, int y, int z) const;

    /**
     * @brief Check if a face should be rendered based on the neighboring block.
     */
    bool shouldRenderFace(int x, int y, int z, int direction, const BlockSampler& sampler) const;

    /**
     * @brief Add a block face to the mesh.
     */
    void addFaceToMesh(std::vector<float>& vertices,
                       std::vector<unsigned int>& indices,
                       int x,
                       int y,
                       int z,
                       int direction,
                       BlockType blockType,
                       const BlockSampler& sampler);

    /**
     * @brief Clean up GPU resources
     */
    void cleanupGraphics();

    /**
     * @brief Initialize OpenGL buffers for mesh data
     */
    void initializeMeshBuffers(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);

    /**
     * @brief Generate vertex data for the chunk mesh.
     */
    void generateVertexData(std::vector<float>& vertices, std::vector<unsigned int>& indices, const BlockSampler& sampler);

    /**
     * @brief Generate index data for the chunk mesh
     * @param indices Output vector for index data
     * @param baseVertex The base vertex index to start from
     * @param vertexCount The number of vertices in the face
     */
    void generateIndexData(std::vector<unsigned int>& indices, unsigned int baseVertex, unsigned int vertexCount);

    /**
     * @brief Upload mesh data to GPU
     * @param vertices The vertex data to upload
     * @param indices The index data to upload
     */
    void uploadMeshToGPU(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);

    // The position of this chunk in chunk coordinates
    glm::ivec3 m_position;

    // 3D array of blocks stored in a 1D array
    std::array<BlockType, CHUNK_VOLUME> m_blocks;

    // Graphics resources
    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_ebo;
    unsigned int m_indexCount;

    // State flags
    bool m_isEmpty;    // True if the chunk contains only air blocks
    bool m_isModified; // True if the chunk has been modified since last mesh generation
    bool m_hasMesh;    // True if the chunk has a generated mesh

    /**
     * @brief Calculate vertex positions for a face
     * @param x Block X coordinate
     * @param y Block Y coordinate
     * @param z Block Z coordinate
     * @param direction Face direction
     * @param i Vertex index (0-3)
     * @return The vertex position
     */
    glm::vec3 calculateVertexPositions(int x, int y, int z, int direction, int i) const;

    /**
     * @brief Calculate vertex colors with per-vertex variation
     * @param blockType The type of block
     * @param vx_local Local X coordinate of vertex
     * @param vy_local Local Y coordinate of vertex
     * @param vz_local Local Z coordinate of vertex
     * @return The vertex color
     */
    glm::vec3 calculateVertexColors(BlockType blockType, int vx_local, int vy_local, int vz_local) const;

    /**
     * @brief Get normal vector for a face
     * @param direction Face direction
     * @return The normal vector
     */
    glm::vec3 calculateVertexNormals(int direction) const;

    /**
     * @brief Calculate ambient occlusion for a vertex. Samples the 8 voxel
     *        positions that share this vertex; handles diagonal neighbors
     *        because the sampler resolves any chunk-local coordinate.
     */
    float calculateVertexAO(int vx_local, int vy_local, int vz_local, const BlockSampler& sampler) const;
};

} // namespace Sylva
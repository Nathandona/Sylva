#pragma once

#include <glm/glm.hpp>

namespace Sylva {

/**
 * @brief Block types in the voxel world
 */
enum class BlockType {
    AIR,    // Empty block (transparent)
    GRASS,  // Green top, dirt sides
    DIRT,   // Basic dirt block
    STONE,  // Basic stone block
    SAND,   // Sand block
    WOOD,   // Tree trunk
    LEAVES, // Tree leaves
    WATER,  // Water block (transparent/translucent)
    COUNT   // Used to get the number of block types
};

/**
 * @brief Structure to hold block appearance data
 */
struct BlockAppearance {
    glm::vec3 color;       // Base color for the block
    float opacity = 1.0f;  // 0.0 = fully transparent, 1.0 = fully opaque
    bool emissive = false; // Whether the block emits light

    // Default constructor for a solid white block
    BlockAppearance() : color(1.0f, 1.0f, 1.0f) {}

    // Constructor with parameters
    explicit BlockAppearance(const glm::vec3& col, float op = 1.0f, bool emit = false)
        : color(col), opacity(op), emissive(emit) {}
};

/**
 * @brief Static block data class
 *
 * Provides global information about block types
 */
class BlockData {
public:
    /**
     * @brief Get the appearance data for a block type
     * @param type Block type to query
     * @return The appearance data
     */
    static const BlockAppearance& getAppearance(BlockType type);

    /**
     * @brief Get the color of a block type
     * @param type Block type to query
     * @return Pointer to an array of 3 floats (RGB)
     */
    static const float* getColor(BlockType type);

    /**
     * @brief Get the color of a block type with per-vertex variation based on world coordinates.
     * @param type Block type to query
     * @param world_vx Absolute world X coordinate of the vertex
     * @param world_vy Absolute world Y coordinate of the vertex
     * @param world_vz Absolute world Z coordinate of the vertex
     * @return The final color as glm::vec3
     */
    static glm::vec3 getBlockColor(BlockType type, int world_vx, int world_vy, int world_vz);

    /**
     * @brief Check if a block type is solid (collidable)
     * @param type Block type to query
     * @return True if the block is solid, false otherwise
     */
    static bool isSolid(BlockType type);

    /**
     * @brief Check if a block type is transparent
     * @param type Block type to query
     * @return True if the block is transparent, false otherwise
     */
    static bool isTransparent(BlockType type);

    /**
     * @brief Initialize block data
     * Call this once during application startup
     */
    static void initialize();

private:
    static BlockAppearance s_appearances[static_cast<size_t>(BlockType::COUNT)];
    static bool s_solid[static_cast<size_t>(BlockType::COUNT)];
    static bool s_transparent[static_cast<size_t>(BlockType::COUNT)];
    static bool s_initialized;
};

} // namespace Sylva
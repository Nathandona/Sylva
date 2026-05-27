#include "block.h"
#include "core/logger.h"

namespace Sylva {

// Initialize static members
BlockAppearance BlockData::s_appearances[static_cast<size_t>(BlockType::COUNT)];
bool BlockData::s_solid[static_cast<size_t>(BlockType::COUNT)];
bool BlockData::s_transparent[static_cast<size_t>(BlockType::COUNT)];
bool BlockData::s_initialized = false;

const BlockAppearance& BlockData::getAppearance(BlockType type) {
    if (!s_initialized) {
        initialize();
    }

    return s_appearances[static_cast<size_t>(type)];
}

const float* BlockData::getColor(BlockType type) {
    if (!s_initialized) {
        initialize();
    }

    return &(s_appearances[static_cast<size_t>(type)].color.r);
}

bool BlockData::isSolid(BlockType type) {
    if (!s_initialized) {
        initialize();
    }

    return s_solid[static_cast<size_t>(type)];
}

bool BlockData::isTransparent(BlockType type) {
    if (!s_initialized) {
        initialize();
    }

    return s_transparent[static_cast<size_t>(type)];
}

void BlockData::initialize() {
    if (s_initialized) {
        return;
    }

    // Set default properties for all blocks
    for (size_t i = 0; i < static_cast<size_t>(BlockType::COUNT); i++) {
        s_appearances[i] = BlockAppearance();
        s_solid[i] = true;
        s_transparent[i] = false;
    }

    // Configure blocks with their specific properties - using refined "pixel clay" aesthetic

    // AIR
    s_appearances[static_cast<size_t>(BlockType::AIR)] = BlockAppearance(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f);
    s_solid[static_cast<size_t>(BlockType::AIR)] = false;
    s_transparent[static_cast<size_t>(BlockType::AIR)] = true;

    // GRASS - Softer, more natural green for micro-voxel terrain
    s_appearances[static_cast<size_t>(BlockType::GRASS)] = BlockAppearance(glm::vec3(0.55f, 0.70f, 0.30f));

    // DIRT - Richer, more textured brown
    s_appearances[static_cast<size_t>(BlockType::DIRT)] = BlockAppearance(glm::vec3(0.65f, 0.45f, 0.30f));

    // STONE - More varied grey with slight blue tint for micro-detail
    s_appearances[static_cast<size_t>(BlockType::STONE)] = BlockAppearance(glm::vec3(0.70f, 0.68f, 0.65f));

    // SAND - Softer, less saturated sand color
    s_appearances[static_cast<size_t>(BlockType::SAND)] = BlockAppearance(glm::vec3(0.88f, 0.81f, 0.64f));

    // WOOD - Deeper, richer wood tone for contrast with terrain
    s_appearances[static_cast<size_t>(BlockType::WOOD)] = BlockAppearance(glm::vec3(0.60f, 0.40f, 0.20f));

    // LEAVES - More varied green with slightly higher opacity
    s_appearances[static_cast<size_t>(BlockType::LEAVES)] = BlockAppearance(glm::vec3(0.40f, 0.65f, 0.28f), 0.85f);
    s_transparent[static_cast<size_t>(BlockType::LEAVES)] = true;

    // WATER - Deeper, more translucent blue
    s_appearances[static_cast<size_t>(BlockType::WATER)] = BlockAppearance(glm::vec3(0.12f, 0.45f, 0.72f), 0.7f);
    s_solid[static_cast<size_t>(BlockType::WATER)] = false;
    s_transparent[static_cast<size_t>(BlockType::WATER)] = true;

    s_initialized = true;
    Logger::logInfo("Block data initialized with micro-voxel appearance");
}

glm::vec3 BlockData::getBlockColor(BlockType type, int world_vx, int world_vy, int world_vz) {
    if (!s_initialized) {
        initialize();
    }

    glm::vec3 const baseColor = s_appearances[static_cast<size_t>(type)].color;

    // Per-vertex subtle hue/color offset variation based on world vertex coordinates
    // Using a simple hash-like function of world vertex coordinates for deterministic randomness
    auto const hash_base_x = static_cast<unsigned int>(world_vx * 73856093);
    auto const hash_base_y = static_cast<unsigned int>(world_vy * 19349663);
    auto const hash_base_z = static_cast<unsigned int>(world_vz * 83492791);

    // Generate small offsets for R, G, B channels
    // The range of offsets should be subtle, e.g., +/- 0.02 to 0.05
    float const r_offset = ((hash_base_x ^ hash_base_y ^ hash_base_z) % 50) / 1000.0f - 0.025f; // Approx [-0.025, +0.025]
    float const g_offset = ((hash_base_x >> 2 ^ hash_base_y >> 1 ^ hash_base_z << 1) % 50) / 1000.0f - 0.025f;
    float const b_offset = ((hash_base_x << 1 ^ hash_base_y >> 2 ^ hash_base_z) % 50) / 1000.0f - 0.025f;

    glm::vec3 finalColor;
    finalColor.r = std::max(0.0f, std::min(1.0f, baseColor.r + r_offset));
    finalColor.g = std::max(0.0f, std::min(1.0f, baseColor.g + g_offset));
    finalColor.b = std::max(0.0f, std::min(1.0f, baseColor.b + b_offset));

    return finalColor;
}

} // namespace Sylva
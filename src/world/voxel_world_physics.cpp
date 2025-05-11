#include "voxel_world_physics.h"
#include "voxel_world.h"
#include "player.h"
#include "core/logger.h"
#include <algorithm>
#include <glm/glm.hpp>

namespace Sylva {

VoxelWorldPhysics::VoxelWorldPhysics() {}

float VoxelWorldPhysics::getHeightAt(const VoxelWorld* world, float x, float z) const {
    float maxHeight = 128.0f;
    float heightIncrement = world->getParams().cellSize;
    for (float y = maxHeight; y >= 0.0f; y -= heightIncrement) {
        glm::vec3 pos(x, y, z);
        BlockType block = world->getBlockAt(pos);
        if (block != BlockType::AIR && BlockData::isSolid(block)) {
            return y + world->getParams().cellSize;
        }
    }
    return 0.0f;
}

bool VoxelWorldPhysics::checkCollision(const VoxelWorld* world, const Player& player) const {
    Vec3 playerPos = player.getPosition();
    float playerSize = player.getCollisionRadius();
    float stepSize = world->getParams().cellSize * 0.5f;
    stepSize = std::max(stepSize, 0.05f);
    bool collision = false;
    for (float y = playerPos.y; y <= playerPos.y + player.getHeight(); y += stepSize) {
        for (float z = playerPos.z - playerSize; z <= playerPos.z + playerSize; z += stepSize) {
            for (float x = playerPos.x - playerSize; x <= playerPos.x + playerSize; x += stepSize) {
                BlockType block = world->getBlockAt(glm::vec3(x, y, z));
                if (block != BlockType::AIR && BlockData::isSolid(block)) {
                    collision = true;
                    if (!world->isCollisionDebugEnabled()) {
                        return true;
                    }
                }
            }
        }
    }
    return collision;
}

} // namespace Sylva 
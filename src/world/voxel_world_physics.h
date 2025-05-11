#pragma once

#include <glm/glm.hpp>
#include "core/types.h"

namespace Sylva {

class Player;
class VoxelWorld;

class VoxelWorldPhysics {
public:
    VoxelWorldPhysics();
    float getHeightAt(const VoxelWorld* world, float x, float z) const;
    bool checkCollision(const VoxelWorld* world, const Player& player) const;
};

} // namespace Sylva
#pragma once

#include <glm/glm.hpp>
#include "core/types.h"

namespace Sylva {

class Camera;
class Player;
class Shader;
class VoxelWorld;

class VoxelWorldRendering {
public:
    VoxelWorldRendering();
    void render(const VoxelWorld* world, const Camera& camera);
    void renderCollisionDebug(VoxelWorld* world, const Camera& camera, const Player& player);
};

} // namespace Sylva 
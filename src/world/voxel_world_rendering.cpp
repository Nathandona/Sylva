#include "voxel_world_rendering.h"
#include "voxel_world.h"
#include "renderer/camera.h"
#include "renderer/shader.h"
#include "core/config.h"
#include "core/logger.h"
#include "player.h"
#include <glad/glad.h>
#include <glm/gtc/noise.hpp>
#include <string>

namespace Sylva {

VoxelWorldRendering::VoxelWorldRendering() {}

void VoxelWorldRendering::render(const VoxelWorld* world, const Camera& camera) {
    Shader* shader = world->getShader();
    if (shader == nullptr) {
        Logger::logWarning("Cannot render voxel world, shader not initialized");
        return;
    }
    glm::mat4 viewMatrix = camera.getViewMatrix();
    glm::mat4 projectionMatrix = camera.getProjectionMatrix(16.0f / 9.0f); // TODO: Get actual aspect ratio
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    shader->use();
    shader->setVec3("lightDir", lightDir);
    glm::vec3 lightColor(
        Config::getFloat("Lighting.light_color.x", 1.0f),
        Config::getFloat("Lighting.light_color.y", 0.95f),
        Config::getFloat("Lighting.light_color.z", 0.8f)
    );
    glm::vec3 ambientColor(
        Config::getFloat("Lighting.ambient_color.x", 0.45f),
        Config::getFloat("Lighting.ambient_color.y", 0.45f),
        Config::getFloat("Lighting.ambient_color.z", 0.45f)
    );
    shader->setVec3("uniformLightColor", lightColor);
    shader->setVec3("uniformAmbientColor", ambientColor);
    for (const auto& pair : world->getChunks()) {
        pair.second->render(shader, viewMatrix, projectionMatrix);
    }
    Logger::logDebug("Voxel world rendered with " + std::to_string(world->getChunks().size()) + " chunks");
}

void VoxelWorldRendering::renderCollisionDebug(VoxelWorld* world, const Camera& camera, const Player& player) {
    world->renderCollisionDebug(camera, player);
}

} // namespace Sylva 
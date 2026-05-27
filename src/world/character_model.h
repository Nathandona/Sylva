#pragma once

#include <glm/glm.hpp>
#include <array>
#include <string>
#include <vector>

namespace Sylva {

class Shader;

/**
 * @brief Animation state driving CharacterModel pose computation.
 *
 * Kept deliberately small — Idle/Walk cover the visible-from-distance cases
 * for a third-person voxel character. Jump can be added by interpolating
 * from a tucked-leg pose.
 */
enum class CharacterAnimState { Idle, Walk };

/**
 * @brief Six-part rigid voxel humanoid: head, torso, arms, legs.
 *
 * Each part is a hand-baked cuboid with its own VAO. Bone pivots are
 * positioned so that arms swing from the shoulder and legs from the hip,
 * not from their geometric center. The model is rendered with the
 * project's existing `colored` shader (per-vertex color, no texture, no
 * lighting) — colors are baked into vertex attributes per face for cheap
 * top/bottom shading.
 *
 * Coordinates are in world units, assuming a 1.8 m tall character standing
 * with feet at y=0 in local space. Player::renderPlayer applies the player
 * translation + yaw on top.
 *
 * Memory: 6 VAOs + 6 VBOs + 6 EBOs. ~50 KB GPU per character.
 */
class CharacterModel {
public:
    CharacterModel();
    ~CharacterModel();

    CharacterModel(const CharacterModel&) = delete;
    CharacterModel& operator=(const CharacterModel&) = delete;
    CharacterModel(CharacterModel&&) = delete;
    CharacterModel& operator=(CharacterModel&&) = delete;

    /**
     * @brief Lazy-build GPU buffers for all body parts. Must be called from
     *        a thread that owns the GL context (typically first call on the
     *        first render frame). Idempotent.
     */
    void ensureMesh();

    /**
     * @brief Advance animation state. Computes per-bone euler rotations from
     *        the current phase + state.
     */
    void update(float deltaTime, CharacterAnimState state);

    /**
     * @brief Draw all parts. The caller supplies the player's root transform
     *        (translate + yaw); this method multiplies each bone's local
     *        transform on top before issuing the draw call.
     */
    void render(Shader& shader,
                const glm::mat4& rootModel,
                const glm::mat4& viewMatrix,
                const glm::mat4& projectionMatrix);

    /**
     * @brief Dense small voxel grid used to author each body part.
     *
     * cells[(y * sz + z) * sx + x] indexes a single voxel cell. Each cell
     * carries a single character code looked up in the part palette
     * (`.` reserved for empty). Authored via human-readable layer strings —
     * see character_model.cpp. Public so anonymous-namespace helpers in the
     * cpp can construct grids.
     */
    struct VoxelGrid {
        int sx{0};
        int sy{0};
        int sz{0};
        std::vector<char> cells;
    };

private:

    /**
     * @brief One rigid body segment with its own GPU mesh + bone state.
     *
     * The part's voxel grid is meshed at bake time: each visible cell face
     * becomes one quad, with neighbor cells inside the same grid culled.
     * Bone pivot is expressed in cell units (`pivotCells`); the baker shifts
     * geometry by -pivot * cellSize so rotation happens at the joint.
     */
    struct BodyPart {
        VoxelGrid grid;
        float cellSize{0.1f};
        glm::vec3 pivotCells{}; // pivot inside the grid, in cell units
        glm::vec3 attachOffset{};
        glm::vec3 restEuler{}; // radians
        glm::vec3 animEuler{}; // radians; updated by update()

        unsigned int vao{0};
        unsigned int vbo{0};
        unsigned int ebo{0};
        unsigned int indexCount{0};
    };

    /**
     * @brief Bake a face-culled voxel mesh for a single body part. Emits
     *        only cell faces that touch an empty neighbor inside the grid;
     *        outer faces are unconditional.
     */
    static void bakeVoxelMesh(BodyPart& part);

    /**
     * @brief Release a part's GL handles. Safe to call on already-cleaned parts.
     */
    static void destroyPart(BodyPart& part);

    enum PartIndex : int {
        Head = 0,
        Torso = 1,
        ArmLeft = 2,
        ArmRight = 3,
        LegLeft = 4,
        LegRight = 5,
        PartCount = 6
    };

    std::array<BodyPart, PartCount> m_parts;

    bool m_built{false};
    float m_animTime{0.0f}; // accumulated seconds, drives sin-curve phase
};

} // namespace Sylva

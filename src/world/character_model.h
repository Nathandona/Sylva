#pragma once

#include <glm/glm.hpp>
#include <array>

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

private:
    /**
     * @brief One rigid body segment with its own GPU mesh + bone state.
     *
     * Geometry is built in the part's *local* space with the bone pivot at
     * the origin. e.g. an arm's vertices live at y in [-armLength, 0] so
     * rotating around the pivot swings the hand, not the shoulder joint.
     *
     * `attachOffset` is where this bone connects to the parent (or root)
     * in the parent's space. `restEuler` is the default pose offset before
     * the animation contribution is added.
     */
    struct BodyPart {
        glm::vec3 size{};        // width, height, depth in world units
        glm::vec3 pivotInGeom{}; // pivot location inside the geometry, world units
        glm::vec3 attachOffset{};
        glm::vec3 restEuler{}; // radians
        glm::vec3 animEuler{}; // radians; updated by update()
        glm::vec3 baseColor{1.0f};
        glm::vec3 topColor{1.0f}; // brighter shade for the upward face

        unsigned int vao{0};
        unsigned int vbo{0};
        unsigned int ebo{0};
        unsigned int indexCount{0};
    };

    /**
     * @brief Bake a cuboid mesh for a single body part. Generates 24 vertices
     *        (4 per face) so each face can carry its own color.
     */
    static void bakeCuboid(BodyPart& part);

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

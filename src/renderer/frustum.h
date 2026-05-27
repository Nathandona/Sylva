#pragma once

#include <array>
#include <glm/glm.hpp>

namespace Sylva {

/**
 * @brief View frustum extracted from a combined view-projection matrix.
 *
 * Used to cheaply cull off-screen chunks before the draw call. Planes are
 * stored as (nx, ny, nz, d) with the normal pointing INTO the frustum, so a
 * point is inside iff dot(plane.xyz, p) + plane.w >= 0.
 *
 * Plane extraction follows Gribb–Hartmann (2001): pluck each plane out of
 * the rows of M = proj * view as (row_w ± row_axis). Works for any
 * projection (perspective or ortho).
 */
class Frustum {
public:
    /**
     * @brief Build a frustum from M = proj * view. Planes are normalized so
     *        the signed-distance test is in world units.
     */
    void extractFromMatrix(const glm::mat4& viewProj);

    /**
     * @brief Axis-aligned bounding box test. true if the AABB intersects or
     *        is fully inside the frustum; false if it's entirely outside any
     *        single plane (guaranteed off-screen). Uses the positive-corner
     *        trick — for each plane, pick the AABB corner furthest along the
     *        plane normal; if that corner is behind the plane, the whole
     *        AABB is behind it.
     */
    bool aabbVisible(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const;

private:
    // Order: Left, Right, Bottom, Top, Near, Far. Stored as (n.xyz, d) with
    // n pointing INTO the frustum.
    std::array<glm::vec4, 6> m_planes{};
};

} // namespace Sylva

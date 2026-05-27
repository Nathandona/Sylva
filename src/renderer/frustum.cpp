#include "frustum.h"

#include <cmath>

namespace Sylva {

namespace {
// Normalize a plane (n.xyz, d) by 1 / length(n.xyz) so signed distance
// becomes world-space distance.
glm::vec4 normalizePlane(const glm::vec4& p) {
    const float len = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    if (len <= 0.0f) {
        return p; // degenerate; avoid div-by-zero
    }
    return p / len;
}
} // namespace

void Frustum::extractFromMatrix(const glm::mat4& m) {
    // glm matrices are column-major; m[col][row]. Gribb-Hartmann needs rows,
    // so build row0..row3 explicitly. Each plane n.xyz + d packed in vec4.
    const glm::vec4 row0(m[0][0], m[1][0], m[2][0], m[3][0]);
    const glm::vec4 row1(m[0][1], m[1][1], m[2][1], m[3][1]);
    const glm::vec4 row2(m[0][2], m[1][2], m[2][2], m[3][2]);
    const glm::vec4 row3(m[0][3], m[1][3], m[2][3], m[3][3]);

    m_planes[0] = normalizePlane(row3 + row0); // Left
    m_planes[1] = normalizePlane(row3 - row0); // Right
    m_planes[2] = normalizePlane(row3 + row1); // Bottom
    m_planes[3] = normalizePlane(row3 - row1); // Top
    m_planes[4] = normalizePlane(row3 + row2); // Near
    m_planes[5] = normalizePlane(row3 - row2); // Far
}

bool Frustum::aabbVisible(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const {
    for (const auto& plane : m_planes) {
        // Positive corner: the AABB vertex furthest along the plane normal.
        // If even that corner is behind the plane, the whole box is behind it.
        const glm::vec3 positive(plane.x >= 0.0f ? aabbMax.x : aabbMin.x,
                                 plane.y >= 0.0f ? aabbMax.y : aabbMin.y,
                                 plane.z >= 0.0f ? aabbMax.z : aabbMin.z);
        const float dist = (plane.x * positive.x) + (plane.y * positive.y) + (plane.z * positive.z) + plane.w;
        if (dist < 0.0f) {
            return false;
        }
    }
    return true;
}

} // namespace Sylva

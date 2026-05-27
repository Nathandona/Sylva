#include "character_model.h"

#include "core/logger.h"
#include "renderer/shader.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Sylva {

namespace {

// Cube World–style chibi proportions: oversized head, compact torso, stubby
// limbs. Total ~1.65 world units tall — a touch shorter than the configured
// Player.height (1.8) but that's the bbox not the visible model. Breakdown:
//   head 0.75  +  torso 0.45  +  leg 0.45  =  1.65
// Limbs are wide for their length so they read as cubes rather than sticks
// at gameplay distance. Head is essentially a giant cube on top of a tiny
// body, the signature Cube World silhouette.
constexpr float kHeadW = 0.75f;
constexpr float kHeadH = 0.70f;
constexpr float kHeadD = 0.70f;
constexpr float kTorsoW = 0.55f;
constexpr float kTorsoH = 0.45f;
constexpr float kTorsoD = 0.35f;
constexpr float kLimbW = 0.22f;
constexpr float kArmH = 0.42f;
constexpr float kArmD = 0.22f;
constexpr float kLegH = 0.45f;
constexpr float kLegD = 0.24f;

constexpr float kLegYTop = kLegH; // hip Y in local space (legs go from 0 → kLegH up)
constexpr float kTorsoYTop = kLegYTop + kTorsoH;

// Palette — desaturated cool tones, distinguishable per part. Top face of each
// part is brightened by `kTopShade` for cheap directional lighting feel.
constexpr float kTopShade = 1.15f;

glm::vec3 brighten(const glm::vec3& c, float k) {
    return glm::vec3(std::min(c.x * k, 1.0f), std::min(c.y * k, 1.0f), std::min(c.z * k, 1.0f));
}

// Push (px,py,pz, r,g,b, u,v) into a float buffer.
void pushVertex(std::vector<float>& buf, float px, float py, float pz, const glm::vec3& c, float u, float v) {
    buf.push_back(px);
    buf.push_back(py);
    buf.push_back(pz);
    buf.push_back(c.r);
    buf.push_back(c.g);
    buf.push_back(c.b);
    buf.push_back(u);
    buf.push_back(v);
}

} // namespace

CharacterModel::CharacterModel() {
    // Head — pivot at the bottom-center so the head rests on top of the torso.
    auto& head = m_parts[Head];
    head.size = glm::vec3(kHeadW, kHeadH, kHeadD);
    head.pivotInGeom = glm::vec3(0.0f, 0.0f, 0.0f); // pivot at base-center
    head.attachOffset = glm::vec3(0.0f, kTorsoYTop, 0.0f);
    head.baseColor = glm::vec3(0.95f, 0.80f, 0.65f); // skin-ish
    head.topColor = brighten(head.baseColor, kTopShade);

    // Torso — pivot at the bottom-center (hip).
    auto& torso = m_parts[Torso];
    torso.size = glm::vec3(kTorsoW, kTorsoH, kTorsoD);
    torso.pivotInGeom = glm::vec3(0.0f, 0.0f, 0.0f);
    torso.attachOffset = glm::vec3(0.0f, kLegYTop, 0.0f);
    torso.baseColor = glm::vec3(0.30f, 0.45f, 0.65f); // shirt blue
    torso.topColor = brighten(torso.baseColor, kTopShade);

    // Arms — pivot at the TOP so rotation swings the hand. Geometry hangs
    // below the pivot (y in [-kArmH, 0]).
    const float armX = (kTorsoW * 0.5f) + (kLimbW * 0.5f);
    auto& armL = m_parts[ArmLeft];
    armL.size = glm::vec3(kLimbW, kArmH, kArmD);
    armL.pivotInGeom = glm::vec3(0.0f, kArmH, 0.0f); // pivot at top of geometry
    armL.attachOffset = glm::vec3(+armX, kTorsoYTop - 0.05f, 0.0f);
    armL.baseColor = glm::vec3(0.95f, 0.80f, 0.65f);
    armL.topColor = brighten(armL.baseColor, kTopShade);

    auto& armR = m_parts[ArmRight];
    armR.size = armL.size;
    armR.pivotInGeom = armL.pivotInGeom;
    armR.attachOffset = glm::vec3(-armX, kTorsoYTop - 0.05f, 0.0f);
    armR.baseColor = armL.baseColor;
    armR.topColor = armL.topColor;

    // Legs — pivot at TOP (hip).
    const float legX = (kTorsoW * 0.25f);
    auto& legL = m_parts[LegLeft];
    legL.size = glm::vec3(kLimbW, kLegH, kLegD);
    legL.pivotInGeom = glm::vec3(0.0f, kLegH, 0.0f);
    legL.attachOffset = glm::vec3(+legX, kLegYTop, 0.0f);
    legL.baseColor = glm::vec3(0.25f, 0.25f, 0.35f); // pants gray-blue
    legL.topColor = brighten(legL.baseColor, kTopShade);

    auto& legR = m_parts[LegRight];
    legR.size = legL.size;
    legR.pivotInGeom = legL.pivotInGeom;
    legR.attachOffset = glm::vec3(-legX, kLegYTop, 0.0f);
    legR.baseColor = legL.baseColor;
    legR.topColor = legL.topColor;
}

CharacterModel::~CharacterModel() {
    for (auto& p : m_parts) {
        destroyPart(p);
    }
}

void CharacterModel::ensureMesh() {
    if (m_built) {
        return;
    }
    for (auto& p : m_parts) {
        bakeCuboid(p);
    }
    m_built = true;
    Logger::logInfo("Character model built (6 parts)");
}

void CharacterModel::bakeCuboid(BodyPart& part) {
    // Build geometry centered in X/Z, with Y running from 0 to size.y, then
    // offset by -pivotInGeom so the bone pivot ends up at the local origin.
    const float hx = part.size.x * 0.5f;
    const float hz = part.size.z * 0.5f;
    const float y0 = 0.0f - part.pivotInGeom.y;
    const float y1 = part.size.y - part.pivotInGeom.y;
    const float xMin = -hx - part.pivotInGeom.x;
    const float xMax = +hx - part.pivotInGeom.x;
    const float zMin = -hz - part.pivotInGeom.z;
    const float zMax = +hz - part.pivotInGeom.z;

    const glm::vec3 cBase = part.baseColor;
    const glm::vec3 cTop = part.topColor;

    std::vector<float> v; // 24 verts × 8 floats = 192 floats
    v.reserve(24 * 8);

    // -Z (back)
    pushVertex(v, xMin, y0, zMin, cBase, 0, 0);
    pushVertex(v, xMax, y0, zMin, cBase, 1, 0);
    pushVertex(v, xMax, y1, zMin, cBase, 1, 1);
    pushVertex(v, xMin, y1, zMin, cBase, 0, 1);
    // +Z (front)
    pushVertex(v, xMax, y0, zMax, cBase, 0, 0);
    pushVertex(v, xMin, y0, zMax, cBase, 1, 0);
    pushVertex(v, xMin, y1, zMax, cBase, 1, 1);
    pushVertex(v, xMax, y1, zMax, cBase, 0, 1);
    // -X (left side)
    pushVertex(v, xMin, y0, zMax, cBase, 0, 0);
    pushVertex(v, xMin, y0, zMin, cBase, 1, 0);
    pushVertex(v, xMin, y1, zMin, cBase, 1, 1);
    pushVertex(v, xMin, y1, zMax, cBase, 0, 1);
    // +X (right side)
    pushVertex(v, xMax, y0, zMin, cBase, 0, 0);
    pushVertex(v, xMax, y0, zMax, cBase, 1, 0);
    pushVertex(v, xMax, y1, zMax, cBase, 1, 1);
    pushVertex(v, xMax, y1, zMin, cBase, 0, 1);
    // -Y (bottom) — slightly darker reads as shadow
    const glm::vec3 cBot = cBase * 0.75f;
    pushVertex(v, xMin, y0, zMax, cBot, 0, 0);
    pushVertex(v, xMax, y0, zMax, cBot, 1, 0);
    pushVertex(v, xMax, y0, zMin, cBot, 1, 1);
    pushVertex(v, xMin, y0, zMin, cBot, 0, 1);
    // +Y (top) — brighter
    pushVertex(v, xMin, y1, zMin, cTop, 0, 0);
    pushVertex(v, xMax, y1, zMin, cTop, 1, 0);
    pushVertex(v, xMax, y1, zMax, cTop, 1, 1);
    pushVertex(v, xMin, y1, zMax, cTop, 0, 1);

    // 6 faces × 2 triangles × 3 indices = 36 indices.
    std::vector<unsigned int> idx;
    idx.reserve(36);
    for (unsigned int face = 0; face < 6; ++face) {
        const unsigned int b = face * 4;
        idx.push_back(b + 0);
        idx.push_back(b + 1);
        idx.push_back(b + 2);
        idx.push_back(b + 0);
        idx.push_back(b + 2);
        idx.push_back(b + 3);
    }
    part.indexCount = static_cast<unsigned int>(idx.size());

    glGenVertexArrays(1, &part.vao);
    glGenBuffers(1, &part.vbo);
    glGenBuffers(1, &part.ebo);

    glBindVertexArray(part.vao);
    glBindBuffer(GL_ARRAY_BUFFER, part.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(v.size() * sizeof(float)), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, part.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(idx.size() * sizeof(unsigned int)),
                 idx.data(), GL_STATIC_DRAW);

    const GLsizei stride = 8 * sizeof(float);
    // GL byte-offset ABI requires int-to-pointer casts. Suppress per call.
    auto offset = [](size_t bytes) -> const void* {
        return reinterpret_cast<const void*>(bytes); // NOLINT(performance-no-int-to-ptr)
    };
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, offset(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, offset(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, offset(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void CharacterModel::destroyPart(BodyPart& part) {
    if (part.ebo != 0) {
        glDeleteBuffers(1, &part.ebo);
        part.ebo = 0;
    }
    if (part.vbo != 0) {
        glDeleteBuffers(1, &part.vbo);
        part.vbo = 0;
    }
    if (part.vao != 0) {
        glDeleteVertexArrays(1, &part.vao);
        part.vao = 0;
    }
    part.indexCount = 0;
}

void CharacterModel::update(float deltaTime, CharacterAnimState state) {
    m_animTime += deltaTime;

    // Reset to rest pose; per-state code below adds animation on top.
    for (auto& p : m_parts) {
        p.animEuler = glm::vec3(0.0f);
    }

    if (state == CharacterAnimState::Walk) {
        // ~1.6 Hz step cadence — close to a brisk human walk and looks
        // natural at the default move speed. Arms swing opposite to legs;
        // left-leg-forward pairs with right-arm-forward, like real walking.
        const float phase = m_animTime * 6.28318f * 1.6f;
        const float legSwing = std::sin(phase) * 0.6f;        // ~34° peak
        const float armSwing = std::sin(phase + 3.14159f) * 0.5f;

        m_parts[LegLeft].animEuler.x = +legSwing;
        m_parts[LegRight].animEuler.x = -legSwing;
        m_parts[ArmLeft].animEuler.x = -legSwing; // opposite to same-side leg
        m_parts[ArmRight].animEuler.x = +legSwing;

        // Tiny torso/head counter-bob so the chest sells the motion.
        m_parts[Torso].animEuler.x = std::sin(phase * 2.0f) * 0.04f;
        m_parts[Head].animEuler.x = std::sin(phase * 2.0f) * 0.02f;
    } else {
        // Idle — gentle breath: torso bob and minute arm sway. Subtle enough
        // that a static screenshot still reads as "standing", not "wiggling".
        const float breath = std::sin(m_animTime * 1.6f);
        m_parts[Head].animEuler.x = breath * 0.02f;
        m_parts[ArmLeft].animEuler.z = +0.05f + breath * 0.01f;
        m_parts[ArmRight].animEuler.z = -0.05f - breath * 0.01f;
    }
}

void CharacterModel::render(Shader& shader,
                            const glm::mat4& rootModel,
                            const glm::mat4& viewMatrix,
                            const glm::mat4& projectionMatrix) {
    ensureMesh();
    shader.use();
    shader.setMat4("view", viewMatrix);
    shader.setMat4("projection", projectionMatrix);

    for (const auto& part : m_parts) {
        if (part.indexCount == 0) {
            continue;
        }
        glm::mat4 m = rootModel;
        m = glm::translate(m, part.attachOffset);
        const glm::vec3 e = part.restEuler + part.animEuler;
        // Apply YXZ Euler — pitch (X) is what we actually use for walk; the
        // other axes are reserved for idle sway / future poses.
        m = glm::rotate(m, e.y, glm::vec3(0, 1, 0));
        m = glm::rotate(m, e.x, glm::vec3(1, 0, 0));
        m = glm::rotate(m, e.z, glm::vec3(0, 0, 1));

        shader.setMat4("model", m);
        glBindVertexArray(part.vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(part.indexCount), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
}

} // namespace Sylva

#include "character_model.h"

#include "core/logger.h"
#include "renderer/shader.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

namespace Sylva {

namespace {

// =============================================================================
// Palette — single-character codes used by the authored voxel grids below.
// '.' reserved for "empty" (no cell). Anything unrecognized renders magenta
// so authoring typos jump out immediately on screen.
// =============================================================================
glm::vec3 palette(char c) {
    switch (c) {
    case 'Y':
        return {0.96f, 0.78f, 0.28f}; // blond hair
    case 'S':
        return {0.96f, 0.80f, 0.62f}; // skin
    case 'B':
        return {0.10f, 0.28f, 0.60f}; // eye blue
    case 'M':
        return {0.55f, 0.20f, 0.20f}; // mouth
    case 'P':
        return {0.45f, 0.32f, 0.55f}; // purple shirt
    case 'D':
        return {0.32f, 0.22f, 0.42f}; // dark purple pants
    case 'G':
        return {0.30f, 0.55f, 0.30f}; // green collar
    case 'W':
        return {0.92f, 0.92f, 0.92f}; // white belt
    case 'K':
        return {0.10f, 0.10f, 0.12f}; // black shoe
    case 'L':
        return {0.65f, 0.65f, 0.68f}; // grey sole
    default:
        return {1.0f, 0.0f, 1.0f}; // magenta: visible error
    }
}

// =============================================================================
// Voxel grid authoring + access.
//
// Layers are written as a vector of Y slices (bottom -> top), each slice is a
// vector of Z rows (front -> back), each row is a string of X cells. So
// `layers[y][z][x]` is the cell character at (x,y,z). Front of the character
// sits at z=0 — that's where eyes/mouth appear on the head, the collar tip on
// the torso, etc.
// =============================================================================
using Layers = std::vector<std::vector<std::string>>;

CharacterModel::VoxelGrid parseGrid(const Layers& layers) {
    CharacterModel::VoxelGrid g;
    g.sy = static_cast<int>(layers.size());
    g.sz = layers.empty() ? 0 : static_cast<int>(layers[0].size());
    g.sx = (g.sz > 0) ? static_cast<int>(layers[0][0].size()) : 0;
    g.cells.assign(static_cast<size_t>(g.sx * g.sy * g.sz), '.');
    for (int y = 0; y < g.sy; ++y) {
        for (int z = 0; z < g.sz; ++z) {
            const std::string& row = layers[y][z];
            for (int x = 0; x < g.sx; ++x) {
                const char c = (x < static_cast<int>(row.size())) ? row[x] : '.';
                g.cells[(y * g.sz + z) * g.sx + x] = c;
            }
        }
    }
    return g;
}

char cellAt(const CharacterModel::VoxelGrid& g, int x, int y, int z) {
    if (x < 0 || x >= g.sx || y < 0 || y >= g.sy || z < 0 || z >= g.sz) {
        return '.';
    }
    return g.cells[(y * g.sz + z) * g.sx + x];
}

// Push a positioned (px,py,pz, r,g,b, u,v) vertex onto a stride-8 buffer.
void pushVert(std::vector<float>& v, float px, float py, float pz, const glm::vec3& c) {
    v.push_back(px);
    v.push_back(py);
    v.push_back(pz);
    v.push_back(c.r);
    v.push_back(c.g);
    v.push_back(c.b);
    v.push_back(0.0f); // u (unused by colored.frag)
    v.push_back(0.0f); // v
}

// Emit one quad face for a unit cube whose -X-Y-Z corner sits at (x0,y0,z0)
// with side length `cs`. `face` is 0..5 — see FACE table below.
void emitFace(std::vector<float>& verts,
              std::vector<unsigned int>& indices,
              float x0,
              float y0,
              float z0,
              float cs,
              int face,
              const glm::vec3& color) {
    // Per-face shading: top brighter, bottom darker, sides as-is. Cheap fake
    // directional lighting without paying for a real lighting pass.
    glm::vec3 c = color;
    if (face == 2) { // +Y top
        c = glm::min(c * 1.18f, glm::vec3(1.0f));
    } else if (face == 3) { // -Y bottom
        c *= 0.70f;
    }

    const float xMin = x0;
    const float xMax = x0 + cs;
    const float yMin = y0;
    const float yMax = y0 + cs;
    const float zMin = z0;
    const float zMax = z0 + cs;

    const auto baseIndex = static_cast<unsigned int>(verts.size() / 8);

    switch (face) {
    case 0: // +X
        pushVert(verts, xMax, yMin, zMin, c);
        pushVert(verts, xMax, yMax, zMin, c);
        pushVert(verts, xMax, yMax, zMax, c);
        pushVert(verts, xMax, yMin, zMax, c);
        break;
    case 1: // -X
        pushVert(verts, xMin, yMin, zMax, c);
        pushVert(verts, xMin, yMax, zMax, c);
        pushVert(verts, xMin, yMax, zMin, c);
        pushVert(verts, xMin, yMin, zMin, c);
        break;
    case 2: // +Y
        pushVert(verts, xMin, yMax, zMin, c);
        pushVert(verts, xMin, yMax, zMax, c);
        pushVert(verts, xMax, yMax, zMax, c);
        pushVert(verts, xMax, yMax, zMin, c);
        break;
    case 3: // -Y
        pushVert(verts, xMin, yMin, zMax, c);
        pushVert(verts, xMin, yMin, zMin, c);
        pushVert(verts, xMax, yMin, zMin, c);
        pushVert(verts, xMax, yMin, zMax, c);
        break;
    case 4: // +Z
        pushVert(verts, xMax, yMin, zMax, c);
        pushVert(verts, xMax, yMax, zMax, c);
        pushVert(verts, xMin, yMax, zMax, c);
        pushVert(verts, xMin, yMin, zMax, c);
        break;
    case 5: // -Z
        pushVert(verts, xMin, yMin, zMin, c);
        pushVert(verts, xMin, yMax, zMin, c);
        pushVert(verts, xMax, yMax, zMin, c);
        pushVert(verts, xMax, yMin, zMin, c);
        break;
    default:
        return;
    }

    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

// =============================================================================
// Cube World-style part grids. Front of the character is z=0; eyes / mouth
// live there. Hair wraps around the upper rear of the head; bangs cover the
// forehead. Sleeves end above the wrist so hands read as skin-colored fists.
// =============================================================================

const Layers& headLayers() {
    // 5 wide × 6 tall × 5 deep.
    // Y=0 (chin), Y=1 (mouth), Y=2 (cheek), Y=3 (eyes), Y=4 (forehead/bangs),
    // Y=5 (hair crown).
    static const Layers layers = {
        // Y = 0 — chin row, slim
        {".SSS.", ".SSS.", ".SSS.", ".SSS.", ".SSS."},
        // Y = 1 — mouth on the front face (z=0)
        {"SSMSS", "SSSSS", "SSSSS", "SSSSS", "SSSSS"},
        // Y = 2 — cheek row
        {"SSSSS", "SSSSS", "SSSSS", "SSSSS", "SSSSS"},
        // Y = 3 — eyes on the front face, hair at the back
        {"SBSBS", "SSSSS", "SSSSS", "SSSSS", "YYYYY"},
        // Y = 4 — bangs covering forehead + hair wrap around skull
        {"YYYYY", "YSSSY", "YSSSY", "YYYYY", "YYYYY"},
        // Y = 5 — full hair crown
        {"YYYYY", "YYYYY", "YYYYY", "YYYYY", "YYYYY"},
    };
    return layers;
}

const Layers& torsoLayers() {
    // 4 wide × 4 tall × 3 deep.
    // Y=0 belt, Y=1-2 shirt, Y=3 collar (green tip on the front).
    static const Layers layers = {
        {"WWWW", "WWWW", "WWWW"},
        {"PPPP", "PPPP", "PPPP"},
        {"PPPP", "PPPP", "PPPP"},
        {"PGGP", "PPPP", "PPPP"},
    };
    return layers;
}

const Layers& armLayers() {
    // 2 wide × 3 tall × 2 deep. Bottom is fist (skin), top is shirt sleeve.
    static const Layers layers = {
        {"SS", "SS"}, // Y = 0 — fist
        {"SS", "SS"}, // Y = 1 — forearm / wrist
        {"PP", "PP"}, // Y = 2 — sleeve
    };
    return layers;
}

const Layers& legLayers() {
    // 2 wide × 3 tall × 2 deep. Bottom is shoe, upper is pants.
    static const Layers layers = {
        {"KK", "KK"}, // Y = 0 — shoe
        {"DD", "DD"}, // Y = 1 — lower pant
        {"DD", "DD"}, // Y = 2 — upper pant
    };
    return layers;
}

} // namespace

CharacterModel::CharacterModel() {
    // -----------------------------------------------------------------
    // Wire each part: voxel grid, cell size, pivot location, and where it
    // attaches to the parent (player root). Pivots are in cell units so the
    // bone rotates around the right joint regardless of resolution choice.
    // -----------------------------------------------------------------
    constexpr float kHeadCell = 0.14f;
    constexpr float kTorsoCell = 0.14f;
    constexpr float kLimbCell = 0.11f;

    // Head — sits on top of torso. Pivot at base-center (chin).
    auto& head = m_parts[Head];
    head.grid = parseGrid(headLayers());
    head.cellSize = kHeadCell;
    head.pivotCells = glm::vec3(2.5f, 0.0f, 2.5f); // center of 5x5 base

    // Torso — sits on top of legs. Pivot at base-center (waist).
    auto& torso = m_parts[Torso];
    torso.grid = parseGrid(torsoLayers());
    torso.cellSize = kTorsoCell;
    torso.pivotCells = glm::vec3(2.0f, 0.0f, 1.5f);
    const float torsoH = static_cast<float>(torso.grid.sy) * torso.cellSize;
    const float torsoW = static_cast<float>(torso.grid.sx) * torso.cellSize;
    const float torsoD = static_cast<float>(torso.grid.sz) * torso.cellSize;

    head.attachOffset = glm::vec3(0.0f, torsoH, 0.0f);

    // Arms — pivot at the TOP (shoulder) so rotation swings the fist.
    auto& armL = m_parts[ArmLeft];
    armL.grid = parseGrid(armLayers());
    armL.cellSize = kLimbCell;
    armL.pivotCells = glm::vec3(1.0f, static_cast<float>(armL.grid.sy), 1.0f);
    const float armW = static_cast<float>(armL.grid.sx) * armL.cellSize;
    // Shoulder sits just outside the torso, near the collar.
    const float shoulderX = (torsoW * 0.5f) + (armW * 0.5f) - 0.02f;
    armL.attachOffset = glm::vec3(+shoulderX, torsoH - 0.04f, 0.0f);

    auto& armR = m_parts[ArmRight];
    armR.grid = armL.grid;
    armR.cellSize = armL.cellSize;
    armR.pivotCells = armL.pivotCells;
    armR.attachOffset = glm::vec3(-shoulderX, torsoH - 0.04f, 0.0f);

    // Legs — pivot at TOP (hip).
    auto& legL = m_parts[LegLeft];
    legL.grid = parseGrid(legLayers());
    legL.cellSize = kLimbCell;
    legL.pivotCells = glm::vec3(1.0f, static_cast<float>(legL.grid.sy), 1.0f);
    const float legW = static_cast<float>(legL.grid.sx) * legL.cellSize;
    const float hipX = (torsoW * 0.25f) + (legW * 0.1f);
    legL.attachOffset = glm::vec3(+hipX, 0.0f, 0.0f);

    auto& legR = m_parts[LegRight];
    legR.grid = legL.grid;
    legR.cellSize = legL.cellSize;
    legR.pivotCells = legL.pivotCells;
    legR.attachOffset = glm::vec3(-hipX, 0.0f, 0.0f);

    // Torso sits at top of legs.
    const float legH = static_cast<float>(legL.grid.sy) * legL.cellSize;
    torso.attachOffset = glm::vec3(0.0f, legH, 0.0f);
    (void)torsoD;
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
        bakeVoxelMesh(p);
    }
    m_built = true;
    Logger::logInfo("Character model built (6 voxel parts)");
}

void CharacterModel::bakeVoxelMesh(BodyPart& part) {
    const auto& g = part.grid;
    const float cs = part.cellSize;

    // Shift so the bone pivot sits at the local origin. Cell (x,y,z)'s
    // lower-corner in part-local space is then (x - pivotX) * cs, etc.
    const float ox = -part.pivotCells.x * cs;
    const float oy = -part.pivotCells.y * cs;
    const float oz = -part.pivotCells.z * cs;

    std::vector<float> verts;
    std::vector<unsigned int> indices;
    // Rough reserve: typical face count is ~30% of the surface-area bound.
    const int cellCount = g.sx * g.sy * g.sz;
    verts.reserve(static_cast<size_t>(cellCount) * 12);
    indices.reserve(static_cast<size_t>(cellCount) * 8);

    static constexpr int OFFSETS[6][3] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
    };

    for (int y = 0; y < g.sy; ++y) {
        for (int z = 0; z < g.sz; ++z) {
            for (int x = 0; x < g.sx; ++x) {
                const char ch = cellAt(g, x, y, z);
                if (ch == '.') {
                    continue;
                }
                const glm::vec3 color = palette(ch);
                const float x0 = ox + static_cast<float>(x) * cs;
                const float y0 = oy + static_cast<float>(y) * cs;
                const float z0 = oz + static_cast<float>(z) * cs;
                for (int face = 0; face < 6; ++face) {
                    // Face is visible iff the neighbor cell is empty. Outside-
                    // the-grid neighbors count as empty, so outer faces are
                    // unconditional.
                    const int nx = x + OFFSETS[face][0];
                    const int ny = y + OFFSETS[face][1];
                    const int nz = z + OFFSETS[face][2];
                    if (cellAt(g, nx, ny, nz) != '.') {
                        continue;
                    }
                    emitFace(verts, indices, x0, y0, z0, cs, face, color);
                }
            }
        }
    }
    part.indexCount = static_cast<unsigned int>(indices.size());

    glGenVertexArrays(1, &part.vao);
    glGenBuffers(1, &part.vbo);
    glGenBuffers(1, &part.ebo);

    glBindVertexArray(part.vao);
    glBindBuffer(GL_ARRAY_BUFFER, part.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(float)), verts.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, part.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    const GLsizei stride = 8 * sizeof(float);
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

    for (auto& p : m_parts) {
        p.animEuler = glm::vec3(0.0f);
    }

    if (state == CharacterAnimState::Walk) {
        const float phase = m_animTime * 6.28318f * 1.6f;
        const float legSwing = std::sin(phase) * 0.6f;
        m_parts[LegLeft].animEuler.x = +legSwing;
        m_parts[LegRight].animEuler.x = -legSwing;
        m_parts[ArmLeft].animEuler.x = -legSwing;
        m_parts[ArmRight].animEuler.x = +legSwing;

        m_parts[Torso].animEuler.x = std::sin(phase * 2.0f) * 0.04f;
        m_parts[Head].animEuler.x = std::sin(phase * 2.0f) * 0.02f;
    } else {
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

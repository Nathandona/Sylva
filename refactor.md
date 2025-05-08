# Sylva Engine Refactoring & Optimization Plan

## 1. Introduction

This document outlines a plan for refactoring and optimizing the Sylva Engine codebase. The goals are to improve code organization, enhance performance, increase maintainability, and lay a solid foundation for future development as detailed in `Plan.md`. This plan considers the existing `src/`, `assets/`, and `config/` structures, as well as the guidelines in `architecture-guidelines.mdc` and `c++.mdc`.

## 2. Code Organization & Structure

Adherence to `architecture-guidelines.mdc` is key.

### 2.1. Module Responsibilities & Granularity

*   **Review `VoxelWorld` Class:**
    *   **Current State:** `VoxelWorld` handles world generation, chunk management, rendering calls for chunks, collision detection, and debug visualizations.
    *   **Suggestion:**
        *   Extract **Terrain Generation Logic**: Create a dedicated `TerrainGenerator` class (e.g., `src/world/terrain_generator.h/.cpp`). This class would encapsulate noise functions, biome logic, and feature placement algorithms (trees, rocks, etc.). `VoxelWorld` would then use an instance of `TerrainGenerator`.
            *   **Benefit:** Decouples terrain algorithms from world representation and management, making both easier to modify and test.
        *   Extract **Chunk Meshing Logic**: If not already highly self-contained within the `Chunk` class, ensure that all mesh generation (greedy meshing, vertex attribute calculation like AO) is fully encapsulated within `Chunk::generateMesh()` or helper methods called by it.
            *   **Benefit:** `Chunk` becomes fully responsible for its visual representation.
*   **Player Class (`src/world/player.h/.cpp`):**
    *   **Current State:** Handles movement, rendering, collision radius, parameters.
    *   **Suggestion:** Ensure rendering logic is strictly about submitting draw data. If it currently involves direct OpenGL calls for setting up VAO/VBOs specifically for the player shape, consider if this could be managed by a more generic `Mesh` or `Renderable` component/class that the `Player` class owns or references.
        *   **Benefit:** Player class focuses on game logic, rendering details are abstracted.
*   **Camera Class (`src/renderer/camera.h/.cpp`):**
    *   **Current State:** Handles view/projection matrices, orbit logic.
    *   **Suggestion:** This seems well-defined. Ensure it remains focused on camera transformations and doesn't creep into input processing (which should be via `InputState`).

### 2.2. Header/Source File Consistency & Forward Declarations

*   **Review Includes:** Minimize includes in header files. Prefer forward declarations where possible to reduce compile times and coupling.
    *   Example: If `VoxelWorld.h` includes `Player.h` only for `VoxelWorld::checkCollision(const Player& player)`, a forward declaration `class Player;` would suffice.
*   **Consistency:** Ensure all classes and free functions follow the `.h`/`.cpp` separation.

### 2.3. Namespace Usage

*   **Current State:** `Sylva` namespace is used.
*   **Suggestion:** Ensure all engine code (core, platform, renderer, world, audio, UI) resides within the `Sylva` namespace or appropriate sub-namespaces (e.g., `Sylva::Audio`, `Sylva::Renderer::UI`). Avoid global namespace pollution.

## 3. Performance Optimizations

### 3.1. Voxel Rendering & World Management

*   **Chunk Meshing (`Chunk::generateMesh`):**
    *   **Greedy Meshing:** `Plan.md` states this is implemented.
        *   **Suggestion:** Profile its performance. Ensure it's optimal for dynamic updates. Are there edge cases or complex scenarios where it could be slower?
    *   **Vertex Data:**
        *   **Suggestion:** Ensure vertex attributes (position, color, UV, normal, AO) are packed efficiently. Consider using smaller data types if precision allows (e.g., `int16_t` for local voxel positions if appropriate).
*   **Level of Detail (LOD) for Chunks (as per `Plan.md` future improvements):**
    *   **Suggestion - Phase 1 (Simplified Meshes):** Generate simplified meshes for distant chunks (e.g., by reducing voxel resolution for the meshing process or using a simpler meshing algorithm).
    *   **Suggestion - Phase 2 (Impostors/Billboards):** For very distant chunks, consider rendering them as 2D impostors or billboards.
    *   **Benefit:** Significantly reduces vertex count for distant terrain.
*   **Frustum Culling:**
    *   **Current State:** `VoxelWorld::render` iterates all chunks.
    *   **Suggestion:** Implement frustum culling for chunks before rendering. Each chunk would have an AABB. Test this AABB against the camera's view frustum.
    *   **Benefit:** Avoids processing and sending data for chunks not visible to the camera.
*   **Occlusion Culling (Advanced, as per `Plan.md`):**
    *   **Suggestion:** Explore software-based occlusion culling (e.g., hierarchical Z-buffer) or hardware occlusion queries (if OpenGL version supports it well and it shows benefits over software methods).
    *   **Benefit:** Avoids rendering chunks hidden behind other opaque geometry (like large hills or other chunks).
*   **Chunk Data Structure (`VoxelWorld::m_chunks`):**
    *   **Current State:** `std::map<glm::ivec3, std::unique_ptr<Chunk>>`.
    *   **Suggestion:** For performance-critical lookups (e.g., `getChunk`), `std::unordered_map` with a custom hash for `glm::ivec3` might be faster than `std::map`. Profile this if chunk lookups become a bottleneck.
        ```cpp
        // Example custom hash for glm::ivec3
        struct IVec3Hash {
            std::size_t operator()(const glm::ivec3& v) const {
                // A common way to combine hashes
                std::size_t h1 = std::hash<int>()(v.x);
                std::size_t h2 = std::hash<int>()(v.y);
                std::size_t h3 = std::hash<int>()(v.z);
                return h1 ^ (h2 << 1) ^ (h3 << 2); 
            }
        };
        // Potentially: std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, IVec3Hash> m_chunks;
        ```
    *   **Benefit:** Potentially faster average-case lookups.

### 3.2. World Generation & Updates (as per `Plan.md`)

*   **Multithreading/Asynchronous Operations:**
    *   **Suggestion - Chunk Generation & Meshing:** Offload terrain data generation (`generateTerrain`, `generateFeatures`) and mesh building (`Chunk::generateMesh`) for new/modified chunks to a thread pool.
        *   When a chunk needs to be loaded/generated, submit a task to the pool.
        *   Once the mesh is ready, flag it for upload to the GPU on the main thread to avoid OpenGL context issues.
    *   **Benefit:** Reduces stuttering and keeps the main loop responsive.
*   **Configuration-Driven Chunk Management:**
    *   **Suggestion:** Make `m_worldSizeInChunks` and `m_viewDistanceInChunks` more dynamic or reloadable from config if they aren't already fully so. This allows for easier tuning. (Currently loaded in constructor).

### 3.3. Collision Detection (`VoxelWorld::checkCollision`)

*   **Current State:** Iterates points within the player's AABB with a step size.
*   **Suggestion:**
    *   **Optimize Sampling:** The current sampling is quite dense. Profile this. Can the step size be slightly increased without missing critical collisions, especially with larger `cellSize`?
    *   **Spatial Partitioning for Broadphase (Advanced):** For a world with many dynamic entities, a broadphase check (e.g., using a simplified grid or octree for collidable objects) could quickly rule out most non-colliding pairs. For player-voxel collision, this is less critical unless many players/NPCs are involved.
    *   **Raycasting Optimizations:** If raycasting is used for other things (mouse picking, AI line of sight), ensure it uses an efficient voxel traversal algorithm (e.g., Amanatides-Woo).
    *   **Benefit:** Faster collision checks.

### 3.4. General C++ Optimizations

*   **Pass by `const&`:** For complex types (structs, classes, `std::string`, vectors) that are read-only in a function, pass them by `const` reference to avoid copy overhead.
*   **Profile-Guided Optimization (PGO):** Use a profiler (e.g., Visual Studio Profiler, Gprof, Perf) to identify actual bottlenecks before spending too much time on micro-optimizations.
*   **Review Loops:** Look for calculations inside loops that can be hoisted out if they are loop-invariant.

## 4. Code Quality & Maintainability

Follow `c++.mdc` guidelines.

### 4.1. Readability & Consistency

*   **Magic Numbers:**
    *   **Suggestion:** Replace unnamed numerical constants with `static constexpr` members, `enum class` values, or named constants defined in a relevant scope (e.g., `config.h`, `types.h`, or within specific classes).
        *   Example: `CHUNK_SIZE` is likely already a constant. Ensure this practice is followed everywhere (e.g., for noise parameters, default sizes, array indices if meaningful).
*   **Function/Method Length:**
    *   **Suggestion:** Break down long functions (e.g., parts of `VoxelWorld::generateTerrain` or `VoxelWorld::generateFeatures`) into smaller, well-named helper methods, even if private.
        *   **Benefit:** Improves readability and testability.
*   **Comments:**
    *   **Suggestion:** Ensure Doxygen-style comments for public APIs. Add comments for complex or non-obvious logic. Remove outdated or misleading comments.

### 4.2. Error Handling & Robustness

*   **Logger:** `Logger` class is in use.
    *   **Suggestion:** Ensure consistent and informative logging. For critical errors that prevent continuation (e.g., shader compilation failure, vital config missing), ensure the program exits gracefully after logging.
*   **Return Values & Optional Data:**
    *   **Suggestion:** For functions that might fail to produce a result or find data (e.g., `Config::get...` if a key is missing, `VoxelWorld::getChunk`), consider using `std::optional` or a clear error/status return mechanism instead of relying on default values or null pointers that might hide issues.
        *   `Config::getInt` currently takes a default. This is okay for many settings, but for critical ones, an optional might be better.
*   **Input Validation:**
    *   **Suggestion:** Add validation for configuration values loaded from files (e.g., reasonable ranges for numbers).

### 4.3. Resource Management (RAII)

*   **Shaders:**
    *   **Current State:** `VoxelWorld` owns `m_shader = new Shader(...)`. `Player` also creates its own shader.
    *   **Suggestion:** Implement a **ShaderManager** (`src/renderer/shader_manager.h/.cpp`).
        *   Responsibilities: Load, compile, link shaders from files. Cache compiled shader programs by name/path. Provide access to shader programs. Manage their lifetime.
        *   Systems (like `VoxelWorld`, `Player`) would request shaders from the manager.
        *   **Benefit:** Avoids redundant shader loading/compilation, centralizes shader resource lifetime.
*   **VAOs/VBOs/EBOs/Textures:**
    *   **Suggestion:** Ensure all OpenGL objects are managed using RAII principles.
        *   Create wrapper classes (e.g., `VertexBuffer`, `VertexArray`, `Texture2D`) that handle `glGen*`, `glBind*`, `glBufferData*`, `glTexImage*` in their constructors/methods and `glDelete*` in their destructors.
        *   **Benefit:** Prevents resource leaks and simplifies cleanup.
*   **Audio System:** Appears to manage its resources well (`AudioSystem::loadSound`, `AudioSystem::shutdown`).

## 5. Shader Code & System

### 5.1. Shader Organization (`assets/shaders/`)

*   **Current State:** `voxel.vert/.frag`, `debug.vert/.frag`, `colored.vert/.frag`, `ui.vert/.frag`, `basic.vert/.frag`.
*   **Suggestion:** This is a reasonable set for now. As more effects or materials are added:
    *   Consider a naming convention (e.g., `material_lightingModel.type`).
    *   Group shaders by purpose if the list grows significantly.

### 5.2. Uniforms & UBOs

*   **Current State:** Uniforms set individually (e.g., `m_shader->setVec3(...)`).
*   **Suggestion:** For common, frequently updated data shared across multiple shaders (like camera projection/view matrices, lighting parameters), consider using **Uniform Buffer Objects (UBOs)**.
    *   Define a C++ struct that mirrors the UBO layout in GLSL.
    *   Update the UBO once per frame. All shaders using it get the updated data.
    *   **Benefit:** Reduces individual `glUniform` calls, potentially improves performance, better organization of shared shader data.

## 6. Asset Management (`assets/`)

### 6.1. Asset Structure

*   **Current State:** `shaders/`, `audio/`, `models/`, `textures/`. This is a good, standard structure.
*   **Suggestion:** As the project grows, consider subdirectories within these if needed (e.g., `assets/textures/environment/`, `assets/textures/character/`).

### 6.2. Asset Loading

*   **Suggestion:**
    *   **Centralized Asset Path Management:** Use the `Config` system or a dedicated path manager to define base paths for assets, making it easier to relocate the `assets` directory or support different asset sources.
    *   **Asynchronous Loading (for large assets):** For larger assets like complex models or many textures, consider loading them on background threads to avoid stalling the main thread. (Similar to chunk loading).

## 7. Build System & Dependencies

### 7.1. CMake & vcpkg

*   **Current State:** `CMakeLists.txt` and `vcpkg.json` are used. Dependencies like GLFW, GLAD, GLM, tinyobjloader are managed via vcpkg, as per `dependencies.mdc`.
*   **Suggestion:**
    *   **Modular CMake:** If `CMakeLists.txt` becomes very large, break it down into smaller `CMakeLists.txt` files per module/directory (`src/core/CMakeLists.txt`, etc.) and use `add_subdirectory()`.
    *   **Build Options:** Use CMake options (e.g., `option(SYLVA_BUILD_TESTS "Build unit tests" ON)`) to control parts of the build, like enabling/disabling tests or optional tools.

## 8. Testing (Not yet in `Plan.md`)

*   **Suggestion:** Introduce a testing framework (e.g., Google Test, Catch2).
    *   **Unit Tests:** For core logic, math functions, configuration parsing, individual components of modules.
        *   Example: Test `Chunk::worldToChunkPos` and `Chunk::chunkToLocalPos` with various inputs. Test noise functions with known seeds for predictable output.
    *   **Integration Tests:** Test interactions between modules.
        *   Example: Test player movement and collision with a mock `VoxelWorld`.
    *   **Benefit:** Improves code reliability, catches regressions early, makes refactoring safer.

## 9. Documentation

### 9.1. Existing Documentation

*   `Plan.md`: Excellent for tracking progress and design decisions.
*   `architecture-guidelines.mdc`, `c++.mdc`, `dependencies.mdc`: Provide solid standards.

### 9.2. In-Code Documentation

*   **Suggestion:** Consistently apply Doxygen-style comments for all public classes, methods, and non-trivial functions, as per `c++.mdc`.
    *   Generate Doxygen HTML documentation periodically.

### 9.3. Design Documents

*   **Suggestion:** For major new systems or significant refactors (like the ones proposed here), create small design documents or update `Plan.md` with the "why" and "how" before implementation.

## 10. Refactoring Process

*   **Iterative Approach:** Refactor one area at a time.
*   **Test After Changes:** If tests are in place, run them frequently. Manually test thoroughly otherwise.
*   **Version Control:** Commit frequently with clear messages. Use branches for larger refactoring tasks.
*   **Prioritize:** Tackle areas that offer the biggest gains in performance or maintainability first, or those that are blocking other development.

This plan provides a comprehensive set of ideas. You can pick and choose which areas to tackle based on your priorities and the current needs of the Sylva Engine. 
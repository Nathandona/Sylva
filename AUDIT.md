# Sylva Engine — Senior Dev Audit

**Date:** 2026-05-27
**Scope:** 6050 LOC, 42 files (`src/`).
**Verdict:** Architectural cosplay. Codebase pretends ECS migration done. Reality: EnTT in vcpkg, four empty folders, monolithic game loop. `Plan.md` and `refactor.md` lie about "Done" status. Fixable, not salvage-tier — but many bad calls accumulated.

---

## CRITICAL (bugs, leaks, broken contracts)

### 1. Phantom ECS
`src/core/ecs/`, `src/core/components/`, `src/world/components/`, `src/world/systems/` are **empty directories**. `Plan.md:21` claims Phase 0 done. `refactor.md` marks ECS-related items "✅ Done". Both lie. EnTT linked but zero `entt::` usage anywhere in code.

### 2. Double shutdown path
`engine.h:24` dtor calls `shutdown()`. `main.cpp:47-48` ALSO calls `engine.shutdown()` then runs dtor. Safe today only because `shutdown()` nulls pointers — accidental safety, not designed. `AudioSystem::shutdown()` lives in `SystemManager::shutdownSystems()` called AFTER engine destruction → asymmetric init/teardown.

### 3. SystemManager is dead weight
`src/system_manager.h/.cpp` = 66 lines for two static functions. Name lies (Plan.md said it would run ECS systems). Today it just bootstraps Config/Logger/GLFW. Either inline into `Engine::initialize` or become a real ECS dispatcher.

### 4. Raw `new`/`delete` in a C++17 project
- `engine.cpp:30-84` — 5 raw owning pointers (window, player, camera, voxelWorld, playerShader).
- `voxel_world.cpp:94` — `m_shader = new Shader(...)`.
- `ui.cpp:87` — `new Shader(...)` never deleted (leak on shutdown).
- `voxel_world.cpp:348-356` — Static local `debugShader = new Shader(...)` never freed.

### 5. Shader leaks GL handle
`shader.h` has **no destructor**. `glDeleteProgram(m_ID)` never called. Every `Shader` constructed = orphaned GPU program at process exit. Rule-of-five missing: copy ctor would double-delete once dtor exists. Must `=delete` copy and provide move.

### 6. Uniform-location thrashing
`shader.cpp:84-116` — every `setVec3/setMat4/...` calls `glGetUniformLocation` again. Driver hash lookup per uniform per frame. Cache in `unordered_map<string, GLint>` in ctor or memoize.

### 7. Audio data race
`audio_system.cpp:481-496` `update()` iterates `activeSounds` while `playSound()` at line 337 inserts into the same static map. No mutex. UB under threading.

### 8. `Config::getFloat` called per-frame, per-voxel-op
`chunk.cpp:368,388,399,410` invoke `Config::getFloat("World.cell_size", ...)` every render and every coordinate conversion. Takes mutex (line 87 of config.cpp), string lookup, conversion. Cache once.

---

## HIGH (architecture rot)

### 9. `VoxelWorld` god class with fake decomposition
`voxel_world.h` 230 lines + `.cpp` 390 lines. Owns: chunks, shader, debug VAO/VBO, generation params, terrain generator, three subsystems, collision debug points. The `voxel_world_generation/physics/rendering.{h,cpp}` files are **17-48 line pass-through wrappers** that call back into `VoxelWorld`:

```cpp
// voxel_world.cpp:175
void VoxelWorld::render(const Camera& camera) {
    m_rendering->render(this, camera);  // pass `this` to subsystem
}
// voxel_world_rendering.cpp:16
void VoxelWorldRendering::render(const VoxelWorld* world, const Camera& camera) {
    Shader* shader = world->getShader();  // ...which calls back
    for (const auto& pair : world->getChunks()) { ... }
}
```

Worst of both worlds: API overhead of decomposition, none of the benefits (no owned state, no testability, no parallel evolution). Either give subsystems real state or fold them back in.

### 10. Duplicate chunk-init logic
`voxel_world.cpp:103-117` (`initializeWorldChunks`) and `:146-164` (`updateChunkLoading`) iterate the same Y -1..7, X/Z ±range loops with the same body (create chunk if missing, generate terrain+features). Extract.

### 11. Duplicate neighbor-mark in `setBlockAt`
`voxel_world.cpp:255-312`. Lines 277-282 set `onBoundary`/`boundaryDirection` then **never use them** — the next block (285-308) redoes 6-axis detection differently. Dead code + duplicated logic.

### 12. 114-line ad-hoc AO neighbor walk
`chunk.cpp:114-228` `calculateVertexAO` manually walks 6 surrounding chunks via raw pointer array. Author's own comment admits diagonals broken. Should be a single `world->getBlockAt(globalPos)` call. Worse: receives `const Chunk* surroundingChunks[6]` — null neighbors not validated at call site (chunk.cpp:534) → UB risk.

### 13. Player ↔ Camera ↔ World hairball
`player.h:52` — `updateMovement(dt, input, world, camera)` takes both world AND camera. Camera for forward/right (`player.cpp:87-88`), world for terrain height + collision (`player.cpp:136,267`). Player class untestable in isolation. Split: input → `MovementIntent` struct → resolver against world.

### 14. Player dead/commented code (year-stale TODO)
`player.h:190-195` — `// unsigned int m_vao;` `// Shader* m_shader;` left as comments with "to be replaced by Mesh component". `player.cpp:17-19, 43-45, 51, 68-83` — entire methods commented out. Either implement the Mesh component or delete the gravestones.

### 15. Hardcoded magic
- `voxel_world.cpp:104, 147` — Y range `-1..7` hardcoded, not configurable.
- `voxel_world_physics.cpp:13` — `maxHeight = 128.0f` hardcoded (ignores `WorldParams::maxHeight`).
- `voxel_world.cpp:366`, `voxel_world_rendering.cpp:23` — `16.0f / 9.0f` aspect ratio hardcoded with own TODO comment.
- `engine.cpp:158` — Footstep every 30 frames (frame-rate dependent!).
- `voxel_world_physics.cpp:29` — `0.05f` minimum step-size magic.

### 16. `getHeightAt` does O(1280) scan per call
`voxel_world_physics.cpp:12-23` — top-down sweep from y=128 by `cellSize=0.1` increments calling `getBlockAt` each step. Called from `player.cpp:136` every frame. Should precompute heightmap per column or query chunk-local topmost solid.

---

## MEDIUM (quality, hygiene, testability)

### 17. Singletons everywhere
`Config`, `Logger`, `Input`, `UI`, `AudioSystem` all static state. No reset, no DI, no mocks. Tests would interfere.

### 18. Logger race
`logger.cpp:42` `setLogLevel` mutates `m_currentLevel` without lock; line 68 reads under lock. Inconsistent. Either lock both or `std::atomic<LogLevel>`.

### 19. Dead Config method
`config.cpp:90` `parseKey` declared, never called. Section.key notation half-stubbed.

### 20. GL leaking into platform/renderer headers
`window.cpp:5`, `input.cpp:3-4`, `ui.cpp:8` include `<glad/glad.h>` directly. Should hide GL behind an abstraction (or accept it and stop pretending modularity).

### 21. Shader exception-swallow
`shader.cpp:42-45` catches file-read failure, logs, returns from ctor with `m_ID` **uninitialized**. Next `use()` → UB. Should throw (and the call sites already `try/catch`).

### 22. Logger boilerplate
`logger.cpp:26-40` — four near-identical `logInfo/logWarning/logError/logDebug` wrappers forwarding to `instance().log()`. Variadic template or kill the wrappers.

### 23. AudioSystem repeated config lookups
`audio_system.cpp:148-161` — 4× same pattern per audio type. Loop over enum.

### 24. `main.cpp:25-32` dead forward decls
`Camera`, `Player`, `VoxelWorld`, `UI::renderCrosshair` forward-declared after headers already include them. Pure noise.

### 25. CMake `file(GLOB_RECURSE ...)`
`CMakeLists.txt:19` — CMake docs explicitly warn against this. New file added → no rebuild trigger. Use explicit list.

### 26. Camera ctor duplication
`camera.cpp:14-20` vs `:35-42` — two ctors duplicate init.

### 27. F1 toggle in main loop
`engine.cpp:132-138` — debug toggle hand-rolled in render loop. Belongs in input or debug system.

---

## LOW

- `engine.cpp:141-157` — repacks `glm::vec3` to `float[3]` every frame. Use `glm::value_ptr`.
- `repomix-output.txt` likely shouldn't be committed; add to `.gitignore`.
- `engine.cpp:174-180` — `1.0f / deltaTime` for FPS doesn't smooth; multi-frame moving average more useful.
- `audio_system.cpp` 591 lines for what should be a thin OpenAL wrapper.

---

## Cleanup applied this pass
1. **Critical bugs/leaks** — Shader RAII, unique_ptr conversions, leak fixes, dead decl removal.
2. **Kill ECS theater** — Empty folders deleted, docs updated to reflect reality.
3. **Collapse VoxelWorld split** — Fake subsystem files folded back, dead code removed.
4. **Dedupe + magic** — Chunk init/neighbor logic deduped, cell_size cached, magic numbers moved to config.

See git log for per-batch commits.

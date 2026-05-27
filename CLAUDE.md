# CLAUDE.md

Engineering cheat-sheet for any agent (or human) opening this repo cold. Reflects the codebase as of 2026-05-27, after the audit cleanup pass.

## What this is

Sylva — voxel game engine in C++17 / OpenGL / OpenAL on Windows. Third-person orbit camera, procedural terrain, single-player. Pre-alpha. ~6000 LOC across `src/`. Built with CMake + vcpkg manifest mode.

Primary user/owner: Nathan Donadey (`Nathandona/Sylva` on GitHub).

## Build / run

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Debug
.\build\Debug\Sylva.exe
```

Reconfigure CMake when source files are **added or removed** — `CMakeLists.txt` uses an explicit source list, not `GLOB_RECURSE`. Editing existing files doesn't need it.

## Architecture in one screen

```
main.cpp          (12 lines)            Engine engine; engine.initialize(); engine.run();
└── Engine                              owns everything below via unique_ptr
    ├── Window (GLFW)
    ├── Camera, Player, VoxelWorld
    ├── IAudioSystem  →  OpenALAudioSystem  (DI: interface + impl)
    ├── UISystem
    └── player Shader

Logger, Config                          service locator: instance state +
                                        static facade. Anywhere can call
                                        Logger::logInfo / Config::getInt
                                        and the call routes to the *current*
                                        instance (default or test-swapped).

Input                                   free functions in Sylva::Input
                                        namespace; static state because
                                        GLFW callbacks need C-linkage.
```

The boundary rules:

- **Engine owns lifetimes.** Every GL/AL handle dies before the GL context does (`m_ui` and `m_playerShader` reset before `m_window`). Order in `Engine::shutdown()` is load-bearing.
- **AudioSystem is interface-driven.** Engine holds `std::unique_ptr<IAudioSystem>`. Callers see `IAudioSystem`, never `OpenALAudioSystem`. Tests can subclass.
- **VoxelWorld owns the chunk map + a `TerrainGenerator`.** `getHeightAt(x,z)` is O(1) — samples the generator's noise, not blocks. Doesn't see player-edited blocks (acceptable until break/place lands).
- **Chunk meshing uses a `BlockSampler` callback** (`std::function<BlockType(int,int,int)>`) passed by `VoxelWorld::updateChunkMeshes`. Sampler resolves any chunk-local offset including diagonals via `getChunk`, so AO corners are correct.
- **Player has no Camera dependency.** `updateMovement(dt, input, fwd, right, world)` takes two vectors, not a Camera ref.

## File layout

```
src/
├── main.cpp                  entry, ~12 lines
├── engine.{h,cpp}            init, tick(), renderFrame(), handleDebugToggles(), logFrameStats()
├── audio/
│   └── audio_system.{h,cpp}  IAudioSystem + OpenALAudioSystem (pImpl) + WAV PCM loader
├── core/
│   ├── config.{h,cpp}        service-locator Config
│   ├── logger.{h,cpp}        service-locator Logger (info/warning/error/debug virtual)
│   └── types.h               Vec3/Mat4 aliases, *Params structs, InputState
├── platform/
│   ├── window.{h,cpp}        GLFW wrapper
│   └── input.{h,cpp}         GLFW callbacks → InputState (free functions)
├── renderer/
│   ├── camera.{h,cpp}        orbit camera, NSDMI defaults
│   ├── shader.{h,cpp}        RAII GL program, uniform-location cache, throws on fail
│   └── ui.{h,cpp}            UISystem class — engine-owned, RAII for VAO/VBO/shader
└── world/
    ├── voxel_world.{h,cpp}   chunks, render, collision, fast getHeightAt
    ├── player.{h,cpp}        movement, physics, collision query
    ├── block.{h,cpp}         BlockType enum + BlockData appearance table
    ├── chunk/
    │   └── chunk.{h,cpp}     storage, culled-face meshing, BlockSampler-driven AO
    └── generation/
        ├── terrain_generator.{h,cpp}   noise composition, sampleHeight
        ├── biome_generator.{h,cpp}     humidity / temperature → block choice
        └── feature_generator.{h,cpp}   trees, rocks, flowers, mushrooms, bushes
```

## Conventions

- **Naming:** PascalCase classes, snake_case functions and locals, `m_` member prefix, `s_` static prefix, `k` for compile-time constants.
- **Namespace:** everything lives under `Sylva`.
- **Headers:** forward-declare where possible; don't pull `<glad/glad.h>` or `<AL/al.h>` into public headers (AL is hidden behind pImpl in `audio_system.cpp`).
- **Comments:** none on the *what*. One short line for the *why* when it's load-bearing (init order, lock scope, magic constant origin). No multi-paragraph docstrings; no comment blocks left behind from removed code.
- **Error handling:** subsystems log + return false / throw at construction. `Shader` throws `std::runtime_error` on compile/link fail. `Logger::logError` does not abort.
- **Threading:** `Logger` uses `std::atomic<LogLevel>` + mutex. `Config` locks the whole `load()` and each accessor. `OpenALAudioSystem` has one mutex around all of its maps + nextSoundId.
- **RAII for every GL/AL handle.** Wrap, don't sprinkle `glDelete*` calls.
- **No raw `new` / `delete`** in engine code. Use `std::make_unique`.

## Service-locator pattern (Logger, Config)

```cpp
// Production code — unchanged:
Logger::logInfo("foo");
Config::getFloat("World.cell_size", 0.25f);

// Tests:
TestLogger fake;            // derives Logger, overrides info() etc.
Logger::setCurrent(&fake);
// ... run code under test
assert(fake.captured.size() == 3);
Logger::setCurrent(nullptr);   // restore default

Config testCfg;
Config::setCurrent(&testCfg);
Config::set("Player.move_speed", 9.0f);
// ... assert behavior
Config::setCurrent(nullptr);
```

A process-wide default exists for each — no null pointer hazard during early bootstrap.

## Audio

- WAV PCM only (mono/stereo, 8-bit or 16-bit). Non-PCM and OGG/MP3 are logged + rejected, not silently filled with garbage.
- Audio asset paths come from `[AudioAssets]` in `default_config.ini`, not hardcoded in engine.
- Background music currently uses a `.ogg` placeholder; loader will skip with a warning until an OGG decoder lands. Game runs silent rather than crashing.

## Coordinate system

- World coordinates are floats. Chunks contain `CHUNK_SIZE = 32` voxels per axis. Chunk model matrix scales voxel-local positions by `World.cell_size` (default 0.25). So voxel (0,0,0) at chunk position (1,0,0) renders at world (8, 0, 0).
- `Chunk::worldToChunkPos` / `worldToLocalPos` handle the conversion; both cache `cell_size` via a function-local static (Config lookup once).
- `TerrainGenerator` works in **voxel space**. `VoxelWorld::getHeightAt` divides world coords by `cell_size` before sampling.

## Tooling traps (encountered on this repo)

- **Windows FS is case-insensitive; git is case-sensitive.** Some files are tracked as `Camera.h` / `Config.h` / `Logger.h` (uppercase) while the disk shows lowercase. `git add src/renderer/camera.h` may silently no-op — always check `git status` after staging, and add with the cased path that matches `git ls-files`.
- **`file(GLOB_RECURSE ...)` in CMake** was previously used and bit us when files were deleted. Replaced with an explicit source list; remember to update it when adding files.
- **Pre-commit line-ending warnings** ("LF will be replaced by CRLF") are harmless; gitattributes haven't been pinned yet.

## Per-batch audit history

The repo's recent commits (`ab13f96`..`HEAD`) are a contiguous cleanup sweep. Read [`AUDIT.md`](AUDIT.md) for the original findings. Highlights of what's been fixed:

1. RAII for shaders / unique_ptr ownership / leak fixes.
2. Empty ECS folders deleted; documentation aligned with reality.
3. `voxel_world_generation/physics/rendering` pass-through wrappers folded back into `VoxelWorld`.
4. Duplicate chunk-loading loops + magic numbers (Y range, aspect ratio, footstep cadence) extracted.
5. Shader uniform-location cache; atomic LogLevel; `SystemManager` killed (was 66 lines of static wrappers).
6. Shader fail-loud on compile/link; audio mutex; **Chunk AO via `BlockSampler` callback** (replaced 114-line broken neighbor walk).
7. Player decoupled from Camera; no-op methods removed.
8. Camera ctor dedup via NSDMI; redundant engine audio init removed.
9. Config writes locked; input dedup; `Engine::run()` split into `tick`/`renderFrame`/etc.
10. Dead `TerrainGenerator::getHeightAt` (with a latent bug) deleted; `findGroundY` helper deduped 5 feature-gen scans.
11. Dead UI/chunk constants + per-frame log spam purged.
12. **Real PCM WAV loader**; audio asset paths to config; **O(1) `VoxelWorld::getHeightAt`** via terrain noise.
13. **AudioSystem → `IAudioSystem` interface + `OpenALAudioSystem` impl**; engine owns via `unique_ptr`.
14. **Logger + Config → service locator**; UI → engine-owned `UISystem` class.

## What's intentionally not done

- **EnTT** is linked but unused. Either commit to the ECS migration in [`Plan.md`](Plan.md) or remove the dependency.
- **No tests yet.** The service-locator hooks and `IAudioSystem` are wired so they can be added without further refactor.
- **No OGG decoder.** `background.ogg` fails to load; engine runs silent.
- **`VoxelWorld::getHeightAt` doesn't see player edits.** Fine until break/place gameplay exists; will need a fallback block scan for modified columns.
- **Greedy meshing / frustum culling / LOD** — explicit future work, not present.

## Commit convention

Conventional commits, lowercase type prefix. Examples in `git log`:

```
refactor: shader RAII, unique_ptr ownership, leak fixes
feat: real WAV loader, config-driven audio paths, O(1) getHeightAt
chore: delete empty ECS folders, soften audit on docs
docs: rewrite README with badges, accurate feature list, layout
```

Commits explicitly **do not** include a Claude / AI co-author trailer.

## Plan & roadmap files

- [`AUDIT.md`](AUDIT.md) — original code-quality review.
- [`Plan.md`](Plan.md) — ECS migration plan (not started past Phase 0 / dependency add).
- [`refactor.md`](refactor.md) — earlier refactoring tracker; somewhat stale, see AUDIT.md for current state.
- [`character_design.md`](character_design.md) — narrative / art direction.

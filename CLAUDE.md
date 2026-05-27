# CLAUDE.md

Engineering cheat-sheet for any agent (or human) opening this repo cold.

## What this is

Sylva — voxel game engine in C++17 / OpenGL / OpenAL on Windows. Third-person orbit camera, procedural terrain, single-player. Pre-alpha but visually runnable (terrain + trees + walkable player cube).

Primary user/owner: Nathan Donadey (`Nathandona/Sylva` on GitHub).

## Build / run

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Debug
.\build\Debug\Sylva.exe
```

The build's `add_custom_command(POST_BUILD ...)` copies `assets/` + `config/` next to the .exe automatically, so double-clicking the binary works (CWD = `build/Debug/`).

Reconfigure CMake when source files are **added or removed** — `CMakeLists.txt` uses an explicit source list, not `GLOB_RECURSE`. Editing existing files doesn't need it.

## Lint / format

```powershell
# Format every source file in place:
clang-format -i src\**\*.h src\**\*.cpp

# Lint (needs compile_commands.json + MSVC env):
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake -S . -B build-tidy -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_COMPILER=clang-cl
clang-tidy -p build-tidy --warnings-as-errors='*' src\**\*.cpp
```

CI (`.github/workflows/lint.yml`) runs both on every push / PR. The codebase currently passes `clang-tidy --warnings-as-errors='*'` with zero findings.

## Architecture in one screen

```
main.cpp          (~20 lines, top-level try/catch)
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
- **Engine main loop is split**: `Engine::run` is an 8-line driver. `tick(dt)` does input + physics + audio listener. `renderFrame(aspect)` does GL clear + world + player + UI. `handleDebugToggles` and `logFrameStats` are siblings.
- **AudioSystem is interface-driven.** Engine holds `std::unique_ptr<IAudioSystem>`. Callers see `IAudioSystem`, never `OpenALAudioSystem`. Tests can subclass.
- **VoxelWorld owns the chunk map + a `TerrainGenerator`.** `getHeightAt(x,z)` is O(1) — samples the generator's noise, not blocks. Doesn't see player-edited blocks (acceptable until break/place lands).
- **Chunk meshing uses a `BlockSampler` callback** (`std::function<BlockType(int,int,int)>`) passed by `VoxelWorld::updateChunkMeshes`. Sampler resolves any chunk-local offset incl. diagonals via `getChunk`, so AO corners are correct.
- **Player has no Camera dependency.** `updateMovement(dt, input, fwd, right, world)` takes two basis vectors, not a Camera ref.
- **Camera knows the world.** `Camera::updateOrbit(dt, player, input, world)` raycasts from `m_target` toward the orbit position; opaque solids pull the camera in so the view never ends up inside terrain. Transparent blocks (LEAVES, WATER) explicitly *don't* obstruct the camera.

## File layout

```
src/
├── main.cpp                  entry + top-level try/catch
├── engine.{h,cpp}            init, tick(), renderFrame(), handleDebugToggles(), logFrameStats()
├── audio/
│   └── audio_system.{h,cpp}  IAudioSystem + OpenALAudioSystem (pImpl) + WAV PCM loader
├── core/
│   ├── config.{h,cpp}        service-locator Config (per-instance kv map)
│   ├── logger.{h,cpp}        service-locator Logger (virtual info/warning/error/debug)
│   └── types.h               Vec3/Mat4 aliases, *Params structs, InputState
├── platform/
│   ├── window.{h,cpp}        GLFW wrapper
│   └── input.{h,cpp}         GLFW callbacks → InputState (free functions, accumulating delta)
├── renderer/
│   ├── camera.{h,cpp}        orbit camera, smooth target, world-collision raycast
│   ├── shader.{h,cpp}        RAII GL program, uniform-location cache, throws on compile/link fail
│   └── ui.{h,cpp}            UISystem class — engine-owned, RAII for VAO/VBO/shader
└── world/
    ├── voxel_world.{h,cpp}   chunks, render, collision, fast getHeightAt
    ├── player.{h,cpp}        movement, physics, axis-slide collision, auto-step, owned cube mesh
    ├── block.{h,cpp}         BlockType enum + BlockData appearance table
    ├── chunk/
    │   └── chunk.{h,cpp}     storage, culled-face meshing, BlockSampler-driven AO
    └── generation/
        ├── terrain_generator.{h,cpp}   plateau-quantized noise composition, sampleHeight
        ├── biome_generator.{h,cpp}     humidity / temperature → block choice
        └── feature_generator.{h,cpp}   trees (sphere crown), rocks, flowers, mushrooms, bushes

config/
└── default_config.ini        committed defaults. user_config.ini is local-only override.

.clang-format / .clang-tidy   LLVM-base style + curated lint checks
.github/workflows/lint.yml    CI: format check + clang-tidy with --warnings-as-errors
```

## Conventions

- **Naming:** PascalCase types, camelBack functions, `m_` member prefix, `s_` file-scope-static prefix, `k` for compile-time constants.
- **Namespace:** everything under `Sylva` (and `Sylva::Input` / `Sylva::UI` where it makes sense).
- **Headers:** forward-declare where possible; don't pull `<glad/glad.h>` or `<AL/al.h>` into public headers (AL is hidden behind pImpl in `audio_system.cpp`).
- **Comments:** none on the *what*. One short line for the *why* when it's load-bearing (init order, lock scope, magic constant origin). No multi-paragraph docstrings; no comment blocks left behind from removed code.
- **Error handling:** subsystems log + return false / throw at construction. `Shader` throws `std::runtime_error` on compile/link fail. `Logger::logError` does not abort.
- **Threading:** `Logger` uses `std::atomic<LogLevel>` + mutex. `Config` locks load + each accessor. `OpenALAudioSystem` has one mutex around all of its maps + `nextSoundId`.
- **RAII for every GL/AL handle.** Wrap, don't sprinkle `glDelete*` calls.
- **No raw `new` / `delete`** in engine code. Use `std::make_unique`.
- **Rule of 5** explicitly on every owning class. Non-movable resource holders mark both copy AND move = delete.

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

## Player movement pipeline

```
updateMovement(dt, input, camFwd, camRight, world)
├── handlePlayerInput → moveDirection (horizontal camera-relative)
├── jump key:   isGrounded ? velocity.y = jumpForce, ungrounded
├── desired = updatePosition(dt, moveDir, world)     // gravity + terrain snap + ledge-step kick
└── horizontal motion:
    1. tryMoveTo(desired.x, desired.z, desired.y)    // full XZ
    2. on block: try X-only, then Z-only (axis-slide so corners don't stick)
    3. each tryMoveTo runs auto-step internally (probe at currentY + autoStepHeight)
    4. no horizontal progress → still apply desired.y for gravity / jump
```

Notable behaviors:

- **Auto-step** climbs single-voxel ledges automatically. `Player.auto_step_height` (default 0.30 world-y ≈ 1 voxel at `cell_size=0.25`). Set to 0 to require jumping.
- **Axis-slide** — diagonal-into-corner doesn't stop the player; they slide along the free axis.
- **Ledge fall kick** — `updatePosition` seeds `m_velocity.y = -2.0` the frame the player walks off a ledge so they drop immediately instead of hanging while gravity ramps from zero.
- **Bbox vs terrain** — the player is `width = 1.0` (4 voxels wide at default `cell_size`). Stair-step terrain finer than 4 voxels per step is **physically unwalkable** — the bbox can't fit on the riser. Either shrink `Player.width/depth` or widen terrain plateaus (see next section).

## Terrain — plateau-quantized noise

`TerrainGenerator` samples noise on a coarse XZ grid so adjacent voxel columns inside the same plateau share a height. Default `World.plateau_width_voxels = 4` (matches player width). The result: flat plateaus connected by sharp 1-voxel steps that auto-step can climb.

- `World.plateau_width_voxels = 1` reverts to per-voxel noise (visually smoother, but unwalkable — every voxel transition is a step).
- `World.plateau_width_voxels = 8` exaggerates the terraced look; even easier to traverse.
- `VoxelWorld::getHeightAt` uses the **same quantization** so the player's ground-snap matches the blocks actually placed.

## Camera

Third-person orbit. `Camera::updateOrbit` does, in order:

1. Smooth `m_target` toward `playerPos + targetHeight` using `1 - exp(-dt * k)` damping (`Camera.follow_smoothing`, default 12).
2. Apply mouse yaw / pitch from accumulated `InputState::mouseDelta` (callbacks `+=` instead of `=` so high-poll-rate mice don't drop events on slower render frames).
3. Pitch clamped at `[-1.05, 1.0]` rad (~ -60° to +57°).
4. Compute ideal orbit position from spherical coords.
5. Raycast in 0.1-world-y steps from `m_target` toward the orbit position. First opaque solid (not LEAVES / WATER / AIR) pulls the camera in to `hit - 0.15`. Floor `0.5` so the camera never collapses onto the player.

Mouse wheel zooms via `Camera.zoom_speed` (default 0.3). Right-mouse-drag zoom was removed — it collided with mouse-look.

## Audio

- WAV PCM only (mono/stereo, 8-bit or 16-bit). Non-PCM and OGG/MP3 are logged + rejected, not silently filled with garbage.
- Audio asset paths come from `[AudioAssets]` in `default_config.ini`, not hardcoded.
- Background music currently uses a `.ogg` placeholder; loader skips with a warning until an OGG decoder lands. Game runs silent rather than crashing.

## Coordinate system

- World coordinates are floats. Chunks contain `CHUNK_SIZE = 32` voxels per axis. Chunk model matrix scales voxel-local positions by `World.cell_size` (default `0.25`). So voxel `(0,0,0)` at chunk position `(1,0,0)` renders at world `(8, 0, 0)`.
- `Chunk::worldToChunkPos` / `worldToLocalPos` handle conversion; both cache `cell_size` via a function-local static (Config lookup once).
- `TerrainGenerator` works in **voxel space**. `VoxelWorld::getHeightAt` divides world coords by `cell_size` before sampling.

## Default world params

The shipped `default_config.ini` is tuned for "fast startup, walkable":

| Key | Value | Why |
|-----|-------|-----|
| `World.size_in_chunks` | 3 | (2N+1)² = 49 chunks at spawn. Much bigger → seconds-long blocking gen on the main thread. |
| `World.view_distance` | 3 | streaming radius |
| `World.chunk_y_min/max` | 0/0 | terrain max_height * cell_size = 2.5 world-y fits in one chunk Y span (0..8) |
| `World.cell_size` | 0.25 | voxel side |
| `World.max_height` | 10 | voxel-y, ~2.5 world-y |
| `World.plateau_width_voxels` | 4 | matches player width — walkable steps |
| `Player.rotation_speed` | 15 | rad/s — snappy 180° turn ~0.2s |
| `Player.auto_step_height` | 0.30 | one voxel + margin |
| `Camera.follow_smoothing` | 12 | ~80ms time constant |
| `Camera.zoom_speed` | 0.3 | one wheel notch is sane |

Bigger worlds (`size_in_chunks` ≥ 6) will visibly hang the window during the first-frame gen — multithreaded chunk generation is open work.

## Tooling traps (encountered on this repo)

- **Windows FS is case-insensitive; git is case-sensitive.** Some files are tracked as `Camera.h` / `Config.h` / `Logger.h` (uppercase) while the disk shows lowercase. `git add src/renderer/camera.h` may silently no-op — always check `git status` after staging, and add with the cased path that matches `git ls-files`.
- **`file(GLOB_RECURSE ...)` in CMake** was previously used and bit us when files were deleted. Replaced with an explicit source list; remember to update it when adding files.
- **Pre-commit line-ending warnings** ("LF will be replaced by CRLF") are harmless; gitattributes haven't been pinned yet.
- **clang-tidy needs `clang-cl` + MSVC env** to resolve `<memory>` and other Windows-SDK headers. The CI workflow sets this up via the `ilammy/msvc-dev-cmd` action; locally you have to source `vcvars64.bat` before configuring the Ninja `build-tidy/` directory.

## What's intentionally not done

- **EnTT** is linked but unused. Either commit to an ECS migration (`Plan.md` in the maintainer's local copy) or remove the dependency.
- **No tests yet.** The service-locator hooks and `IAudioSystem` are wired so they can be added without further refactor.
- **No OGG decoder.** `background.ogg` fails to load; engine runs silent.
- **`VoxelWorld::getHeightAt` doesn't see player edits.** Fine until break/place gameplay exists; will need a fallback block scan for modified columns.
- **Greedy meshing / frustum culling / LOD** — explicit future work, not present.
- **Multithreaded chunk gen** — explicit future work. Default world is small so it doesn't matter yet.
- **Player mesh is a placeholder cube.** `ensureMesh()` lazy-builds it on first render; swap for a real Mesh component when ready (color baked from `m_params.color`).
- **No swept collision.** Player position is teleported per frame; very high speeds could tunnel through thin walls.

## Commit convention

Conventional commits, lowercase type prefix. Examples in `git log`:

```
refactor: shader RAII, unique_ptr ownership, leak fixes
feat: real WAV loader, config-driven audio paths, O(1) getHeightAt
feat: tighten camera + player game feel
feat: auto-step for 1-voxel ledges
feat: plateau-quantized terrain for walkable stairs
fix: axis-separated collision slide + cleaner move pipeline
fix: seed fall velocity when stepping off a ledge
style: clang-format pass across src/ (mechanical, no behavior change)
ci: add clang-format + clang-tidy + GitHub Actions workflow
docs: rewrite README with badges, accurate feature list, layout
```

Commits explicitly **do not** include a Claude / AI co-author trailer.

## Documentation

- [`AUDIT.md`](AUDIT.md) — original code-quality review; punch-list status.
- `Plan.md`, `refactor.md`, `character_design.md` — local-only docs (in `.gitignore`). Not part of the public repo. ECS migration plan + lore / narrative direction live there for the maintainer.

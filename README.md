<div align="center">

# Sylva

**A voxel engine for a primal-magic open world.**

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.15%2B-064F8C?style=flat-square&logo=cmake&logoColor=white)](https://cmake.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.x-5586A4?style=flat-square&logo=opengl&logoColor=white)](https://www.opengl.org/)
[![OpenAL](https://img.shields.io/badge/OpenAL-Soft-2C6CB4?style=flat-square&logoColor=white)](https://openal-soft.org/)
[![vcpkg](https://img.shields.io/badge/vcpkg-managed-0078D4?style=flat-square&logo=microsoft&logoColor=white)](https://vcpkg.io/)
[![Platform](https://img.shields.io/badge/platform-Windows-0078D6?style=flat-square&logo=windows&logoColor=white)](#)
[![Status](https://img.shields.io/badge/status-pre--alpha-orange?style=flat-square)](#status)
[![License](https://img.shields.io/badge/license-TBD-lightgrey?style=flat-square)](#license)

</div>

---

Sylva renders a chunked micro-voxel world the player explores in third-person. Terrain is procedurally generated from layered noise, meshed per-chunk with culled faces and per-vertex ambient occlusion (with correct diagonal-neighbor sampling), and lit with a single directional source plus per-biome color variation. The engine is small, dependency-light, and built to grow into a full survival/exploration game — see [`character_design.md`](character_design.md) for the world-building direction.

## Highlights

- **Chunked voxel world** — 32³ chunks, view-distance streaming, neighbor-aware meshing.
- **Procedural terrain** — biome + feature generators on top of multi-octave noise.
- **Real spatial audio** — PCM WAV loader, OpenAL listener tracking the camera, per-type volumes and mute, thread-safe under concurrent playback.
- **Third-person orbit camera** — configurable distance, height, sensitivity.
- **Service-locator architecture** — `Logger` / `Config` are real swappable instances behind a static facade; `IAudioSystem` is an interface with an `OpenALAudioSystem` impl. Tests can substitute fakes without touching call sites.
- **Config-driven** — `default_config.ini` + `user_config.ini` overrides cover window, audio (volumes, mute, asset paths), camera, player, world generation, and debug toggles.
- **Debug overlay** — F1 toggles collision-point visualization in-world.
- **RAII everywhere** — every GL/AL handle (shaders, buffers, sources, the audio context itself) lives in a class whose destructor releases it.

## Tech stack

| Layer        | Library                                                                                                  |
| ------------ | -------------------------------------------------------------------------------------------------------- |
| Windowing    | [GLFW3](https://www.glfw.org/)                                                                           |
| GL loader    | [GLAD](https://glad.dav1d.de/)                                                                           |
| Math         | [GLM](https://github.com/g-truc/glm)                                                                     |
| Model import | [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)                                          |
| Audio        | [OpenAL Soft](https://openal-soft.org/) (with a hand-rolled PCM WAV parser)                              |
| ECS (linked but currently unused) | [EnTT](https://github.com/skypjack/entt)                                            |
| Build        | CMake ≥ 3.15 + [vcpkg](https://vcpkg.io/) manifest mode                                                  |

## Build

**Prerequisites:** Visual Studio 2019+ (or any C++17 compiler), CMake ≥ 3.15, `vcpkg` cloned and bootstrapped, `VCPKG_ROOT` env var set.

```powershell
git clone https://github.com/Nathandona/Sylva.git
cd Sylva

cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Debug

.\build\Debug\Sylva.exe
```

vcpkg pulls every dependency listed in [`vcpkg.json`](vcpkg.json) on first configure. Assets and config files are copied next to the binary automatically.

## Configuration

Two layered INI files:

| File                                              | Purpose                              |
| ------------------------------------------------- | ------------------------------------ |
| [`config/default_config.ini`](config/default_config.ini) | Engine defaults, version-controlled. |
| `config/user_config.ini`                          | Local overrides, not committed.      |

Common knobs: `Window.*`, `Audio.*`, `AudioAssets.*` (sound file paths), `Camera.*`, `Player.*`, `World.cell_size`, `World.view_distance`, `World.chunk_y_min/max`, `Debug.collision_visualization`, `Debug.max_frames`.

## Architecture in 30 seconds

```
main()
  └─ Engine
       ├─ owns Window (GLFW)
       ├─ owns Camera, Player, VoxelWorld
       ├─ owns std::unique_ptr<IAudioSystem>  (concrete: OpenALAudioSystem)
       ├─ owns std::unique_ptr<UISystem>
       └─ owns player Shader

Logger / Config           service locator + static facade — accessible
                           anywhere via Logger::logInfo / Config::getInt;
                           tests swap with setCurrent(&fake)

Input                     free functions (GLFW callbacks need C-linkage)

VoxelWorld                owns a TerrainGenerator + the chunk map. Public
                           getHeightAt is O(1) — samples the generator's
                           noise directly instead of scanning blocks.

Chunk::generateMesh       takes a BlockSampler callback that resolves any
                           chunk-local offset (including diagonals) via
                           the world — so AO is correct at chunk corners.
```

## Project layout

```
src/
├── main.cpp                application entry (12 lines — Engine does the work)
├── engine.{h,cpp}          init, tick / renderFrame / debug toggles / FPS log
├── audio/                  IAudioSystem interface + OpenALAudioSystem impl + WAV loader
├── core/                   Config + Logger (service locator), shared types
├── platform/               window (GLFW), input (free functions)
├── renderer/               camera, shader (RAII + uniform cache), UISystem
└── world/
    ├── voxel_world.{h,cpp} chunk store, render, collision, fast getHeightAt
    ├── player.{h,cpp}      movement + physics (no Camera dependency)
    ├── block.{h,cpp}       block types + palette
    ├── chunk/              storage, culled-face meshing, per-vertex AO
    └── generation/         terrain (noise composition), biome, feature gen

config/                     INI configs (defaults committed)
assets/                     shaders, audio (WAV preferred), textures, models
```

## Controls

| Input              | Action                       |
| ------------------ | ---------------------------- |
| `W` `A` `S` `D`    | Move (camera-relative)       |
| Mouse              | Orbit camera                 |
| Scroll wheel       | Zoom                         |
| `Space`            | Jump                         |
| `F1`               | Toggle collision debug points |
| `Esc`              | Quit                         |

## Status

Pre-alpha. Runnable, hackable, not playable. Active areas:

- ✅ Chunk streaming, meshing, culled-face rendering, per-vertex AO (corners correct)
- ✅ Camera + player controller + voxel collision
- ✅ Config / Logger as swappable services; AudioSystem as DI interface
- ✅ Real PCM WAV playback; thread-safe audio
- ✅ O(1) terrain height query (player ground sample)
- 🚧 Asset & shader managers (currently ad-hoc — engine owns one player shader)
- 🚧 Multithreaded chunk generation
- 🚧 OGG / Vorbis decoder for music (only WAV supported today)
- ⏳ ECS migration (EnTT linked, not wired in — see [`Plan.md`](Plan.md))
- ⏳ Greedy meshing, LOD, frustum culling
- ⏳ Gameplay (combat, crafting, magic, biomes) — see [`character_design.md`](character_design.md)

See [`AUDIT.md`](AUDIT.md) for the original code-quality review and [`CLAUDE.md`](CLAUDE.md) for the architecture / conventions cheat-sheet.

## License

Not yet chosen — all rights reserved until a `LICENSE` file lands.

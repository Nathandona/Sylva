<div align="center">

# Sylva

**A voxel engine for a primal-magic open world.**

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.15%2B-064F8C?style=flat-square&logo=cmake&logoColor=white)](https://cmake.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.x-5586A4?style=flat-square&logo=opengl&logoColor=white)](https://www.opengl.org/)
[![vcpkg](https://img.shields.io/badge/vcpkg-managed-0078D4?style=flat-square&logo=microsoft&logoColor=white)](https://vcpkg.io/)
[![Platform](https://img.shields.io/badge/platform-Windows-0078D6?style=flat-square&logo=windows&logoColor=white)](#)
[![Status](https://img.shields.io/badge/status-pre--alpha-orange?style=flat-square)](#status)
[![License](https://img.shields.io/badge/license-TBD-lightgrey?style=flat-square)](#license)

</div>

---

Sylva renders a chunked micro-voxel world the player explores in third-person. Terrain is procedurally generated from layered noise, meshed per-chunk with culled faces and per-vertex ambient occlusion, and lit with a single directional source plus per-biome color variation. The engine is small, dependency-light, and built to grow into a full survival/exploration game — see [`character_design.md`](character_design.md) for the world-building direction.

## Highlights

- **Chunked voxel world** — 32³ chunks, view-distance streaming, neighbor-aware meshing.
- **Procedural terrain** — biome + feature generators on top of multi-octave noise.
- **Third-person orbit camera** — configurable distance, height, sensitivity.
- **Spatial audio** — OpenAL listener tracks the camera; per-channel volumes and mute.
- **Config-driven** — `default_config.ini` + `user_config.ini` overrides, hot-tweakable.
- **Debug overlay** — F1 toggles collision-point visualization in-world.
- **RAII GL/AL** — shaders, buffers, and audio sources own their handles.

## Tech stack

| Layer        | Library                                                                                                  |
| ------------ | -------------------------------------------------------------------------------------------------------- |
| Windowing    | [GLFW3](https://www.glfw.org/)                                                                           |
| GL loader    | [GLAD](https://glad.dav1d.de/)                                                                           |
| Math         | [GLM](https://github.com/g-truc/glm)                                                                     |
| Model import | [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)                                          |
| Audio        | [OpenAL Soft](https://openal-soft.org/)                                                                  |
| ECS (vendored, unused yet) | [EnTT](https://github.com/skypjack/entt)                                                   |
| Build        | CMake ≥ 3.15 + [vcpkg](https://vcpkg.io/) manifest mode                                                  |

## Build

**Prerequisites:** Visual Studio 2019+ (or any C++17 compiler), CMake ≥ 3.15, `vcpkg` cloned and bootstrapped, `VCPKG_ROOT` env var set.

```powershell
git clone https://github.com/<you>/Sylva.git
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

Common knobs: `Window.*`, `Audio.*`, `World.cell_size`, `World.view_distance`, `World.chunk_y_min/max`, `Player.move_speed`, `Debug.collision_visualization`.

## Project layout

```
src/
├── main.cpp                application entry
├── engine.{h,cpp}          init, main loop, shutdown
├── system_manager.{h,cpp}  global subsystem bootstrap
├── audio/                  OpenAL wrapper
├── core/                   config, logger, shared types
├── platform/               window (GLFW), input
├── renderer/               camera, shader, UI overlay
└── world/
    ├── voxel_world.{h,cpp} chunk store, render, collision
    ├── player.{h,cpp}      player movement + physics
    ├── block.{h,cpp}       block types and palette
    ├── chunk/              chunk storage + meshing
    └── generation/         terrain, biome, feature gen

config/                     INI configs (defaults committed)
assets/                     shaders, audio, textures, models
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

- ✅ Chunk streaming, meshing, culled-face rendering, per-vertex AO
- ✅ Camera + player controller + voxel collision
- ✅ Config / logger / OpenAL audio subsystems
- 🚧 Asset & shader managers (currently ad-hoc)
- 🚧 Multithreaded chunk generation
- ⏳ ECS migration (EnTT linked, not wired in — see [`Plan.md`](Plan.md))
- ⏳ Greedy meshing, LOD, frustum culling
- ⏳ Gameplay (combat, crafting, magic, biomes) — see [`character_design.md`](character_design.md)

See [`AUDIT.md`](AUDIT.md) for the current code-quality punch list and [`refactor.md`](refactor.md) for ongoing cleanup.

## License

Not yet chosen — all rights reserved until a `LICENSE` file lands.

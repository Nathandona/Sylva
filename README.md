# Sylva: Low-Poly Open-World RPG

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) <!-- Optional: Add a license badge if applicable -->

**Sylva** is an in-development open-world RPG inspired by the cozy aesthetics of **Pelican Harbor** and the exploration-driven gameplay of **Cube World**. It's being built from the ground up using C++ and OpenGL.

![Visual Inspiration Placeholder](https://via.placeholder.com/600x300.png?text=Sylva+Visual+Inspiration+%28Low-Poly%2C+Cozy%29)
*(Imagine a blend of Pelican Harbor's soft shading and Cube World's exploration)*

## Concept

-   **Visuals:** Low-poly, cozy 3D art style with simple shaders and colorful OpenGL rendering.
-   **Technology:** C++17 and OpenGL 3.3+, powered by a custom lightweight engine (no Unity/Unreal).
-   **Gameplay:** Open-world RPG focusing on exploration, leveling, loot, and combat.

## MVP (Minimum Viable Product) Scope

The current goal is to build an MVP demonstrating the core concepts:

-   **Rendering:** Basic low-poly models (terrain, characters, simple props), simple directional lighting, third-person camera.
-   **World:** Procedural, chunk-based terrain generation (smooth hills, forests).
-   **Character:** Basic player model, WASD movement, jump, simple attack.
-   **Progression:** Experience points from defeating enemies, simple leveling (health/damage increase).
-   **Enemies:** Simple AI (move towards player, attack when close).
-   **UI:** Health bar, XP bar.
-   **Sound:** Basic background music and sound effects.

## Technical Stack

| Component       | Choice                                  | Notes                                     |
| :-------------- | :-------------------------------------- | :---------------------------------------- |
| Language        | C++17                                   |                                           |
| Graphics API    | OpenGL 3.3+ (Core Profile)              | Using GLFW for windowing/input, GLEW/GLAD |
| Math Library    | GLM                                     | OpenGL Mathematics Library                |
| Build System    | CMake                                   |                                           |
| Package Manager | `vcpkg`                                 | For managing external libraries           |
| Asset Format    | `.obj`                                  | For simple static meshes from Blender     |
| Texture Loading | `stb_image.h`                           | Simple single-header image loader         |
| Audio Library   | OpenAL Soft / SDL2_mixer (TBD)          | Decision pending further research         |
| Input/Windowing | GLFW                                    | Cross-platform window and input handling  |

## Getting Started (Placeholder)

*(Instructions on how to build and run the project will go here once available)*

1.  **Prerequisites:**
    *   C++17 compatible compiler (GCC, Clang, MSVC)
    *   CMake (version X.Y or higher)
    *   `vcpkg`
    *   Git
2.  **Clone the repository:**
    ```bash
    git clone <repository-url>
    cd Sylva
    ```
3.  **Install dependencies using `vcpkg`:**
    ```bash
    vcpkg install glfw3 glm glad # Add other dependencies as needed
    ```
4.  **Configure and build with CMake:**
    ```bash
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
    cmake --build build
    ```
5.  **Run:**
    ```bash
    ./build/bin/Sylva # Adjust executable path as needed
    ```

## Development Roadmap (MVP - Rough Estimate)

| Week | Focus                                               | Status      |
| :--- | :-------------------------------------------------- | :---------- |
| 1    | OpenGL Window Setup, Render Basic Triangle          | To Do       |
| 2    | Model Loading (.obj), Render Static Meshes          | To Do       |
| 3    | Third-Person Camera Implementation (Mouse/Keyboard) | To Do       |
| 4    | Basic Static Terrain System                         | To Do       |
| 5    | Player Movement, Gravity, Simple Collision          | To Do       |
| 6    | Basic Enemy Implementation (Simple AI)              | To Do       |
| 7    | XP & Leveling System                                | To Do       |
| 8    | Basic UI (Health/XP), Sound Integration, Polish     | To Do       |

*(This roadmap is tentative and subject to change)*

## Contributing

*(Contribution guidelines will be added here later)*

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details (assuming MIT, create this file if needed).

# Sylva Engine

A modular 3D game engine with a focus on clean architecture and extensibility.

## Core Systems Implemented

### 1. Logger System
- Centralized logging utility for the engine
- Supports log levels (DEBUG, INFO, WARNING, ERROR)
- Outputs to console and optionally to a file
- Configurable via config file

### 2. Config System
- Loads engine and game settings from .ini files
- Supports default and user-specific overrides
- Provides access to configuration values throughout the engine

### 3. Camera System
- Third-person orbit camera that follows the player
- Configurable parameters for distance, height, and sensitivity
- Exposes camera vectors for gameplay and UI

### 4. Player System
- WASD world-space movement (placeholder implementation)
- Simple white rectangle representation
- Rotation to face movement direction

### 5. World System
- Simple terrain generation
- Height-based rendering (placeholder)
- Collision detection (placeholder)

### 6. UI System
- Screen-centered crosshair aligned with camera's forward vector
- Extendable for additional HUD elements

## Building the Project

### Prerequisites
- CMake 3.15 or higher
- C++17 compatible compiler
- vcpkg package manager

### Dependencies
- GLFW3 - Window management and input handling
- GLAD - OpenGL loader
- GLM - Mathematics library for graphics
- tinyobjloader - Wavefront .obj file loader

### Build Instructions
1. Clone the repository
2. Install dependencies using vcpkg
   ```
   vcpkg install glfw3 glad glm tinyobjloader
   ```
3. Configure with CMake
   ```
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
   ```
4. Build the project
   ```
   cmake --build build
   ```

## Configuration

The engine uses two configuration files:
- `config/default_config.ini` - Default settings
- `config/user_config.ini` - User-specific overrides

## Future Improvements

- Window and input handling (currently stubbed)
- Rendering system with shaders
- Physics integration
- Advanced terrain generation
- Animation system
- Asset management
- Audio system 
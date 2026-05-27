cmake_minimum_required(VERSION 3.15)
project(Sylva VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/src)

# Find dependencies
find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(glm REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(OpenAL REQUIRED)
find_package(EnTT REQUIRED)

# Source files (explicit list — globbing skips rebuild triggers on new files)
set(SOURCES
    src/main.cpp
    src/engine.cpp
    src/audio/audio_system.cpp
    src/core/config.cpp
    src/core/logger.cpp
    src/platform/input.cpp
    src/platform/window.cpp
    src/renderer/camera.cpp
    src/renderer/shader.cpp
    src/renderer/ui.cpp
    src/world/block.cpp
    src/world/player.cpp
    src/world/voxel_world.cpp
    src/world/chunk/chunk.cpp
    src/world/generation/biome_generator.cpp
    src/world/generation/feature_generator.cpp
    src/world/generation/terrain_generator.cpp
)

# Define the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad::glad
    glm::glm
    tinyobjloader::tinyobjloader
    OpenAL::OpenAL
    EnTT::EnTT
)

# Copy assets to the build directory
file(COPY ${PROJECT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR}) 
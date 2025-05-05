cmake_minimum_required(VERSION 3.15)
project(Sylva VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include vcpkg if it's available
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

# Find packages
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Set source files
set(SOURCES
    src/main.cpp
    src/core/Core.cpp
    src/core/Logger.cpp
    src/core/Config.cpp
    src/platform/Platform.cpp
    src/renderer/Renderer.cpp
    src/renderer/Camera.cpp
    src/renderer/CameraController.cpp
    src/renderer/CameraDebug.cpp
    src/renderer/DebugDrawing.cpp
    src/renderer/Shader.cpp
    src/renderer/Texture.cpp
    src/renderer/Mesh.cpp
    src/renderer/Model.cpp
    src/renderer/ResourceManager.cpp
    src/world/World.cpp
    src/world/Terrain.cpp
    src/world/Player.cpp
    src/input/InputManager.cpp
    vendor/stb/stb_image_impl.cpp
)

# Set header files
set(HEADERS
    src/core/Core.h
    src/core/Logger.h
    src/core/Config.h
    src/platform/Platform.h
    src/renderer/Renderer.h
    src/renderer/Camera.h
    src/renderer/CameraController.h
    src/renderer/CameraDebug.h
    src/renderer/DebugDrawing.h
    src/renderer/Shader.h
    src/renderer/Texture.h
    src/renderer/Mesh.h
    src/renderer/Model.h
    src/renderer/ResourceManager.h
    src/world/World.h
    src/world/Terrain.h
    src/world/Player.h
    src/input/InputManager.h
    vendor/stb/stb_image.h
    vendor/tiny_obj_loader/tiny_obj_loader.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad::glad
    glm::glm
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

# Copy assets to build directory during build
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR}) 
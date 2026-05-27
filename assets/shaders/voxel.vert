#version 330 core

// Packed 12-byte vertex from PackedVertex (see chunk.h). Position is in
// chunk-local voxel coords [0, CHUNK_SIZE]; the chunk's model matrix
// translates + scales it into world units. Normal is a face index 0..5 — we
// look the actual vec3 up here so the VBO doesn't have to store it.
layout (location = 0) in uvec3 aPos;
layout (location = 1) in uint aNormal;
layout (location = 2) in vec4 aColor;       // normalized GL_UNSIGNED_BYTE
layout (location = 3) in float aOcclusion;  // normalized GL_UNSIGNED_BYTE

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 ourColor;
out vec3 Normal;
out vec3 FragPos;
out float Occlusion;

const vec3 FACE_NORMALS[6] = vec3[6](
    vec3( 1.0,  0.0,  0.0),
    vec3(-1.0,  0.0,  0.0),
    vec3( 0.0,  1.0,  0.0),
    vec3( 0.0, -1.0,  0.0),
    vec3( 0.0,  0.0,  1.0),
    vec3( 0.0,  0.0, -1.0)
);

void main() {
    vec3 posF = vec3(aPos);

    FragPos = vec3(model * vec4(posF, 1.0));
    gl_Position = projection * view * model * vec4(posF, 1.0);

    ourColor = aColor.rgb;
    Occlusion = aOcclusion;

    // Normal matrix protects against non-uniform model scale. For uniform
    // scales this collapses to mat3(model); we keep the general form so a
    // future per-axis scale doesn't silently break lighting.
    vec3 nObj = FACE_NORMALS[int(aNormal)];
    Normal = mat3(transpose(inverse(model))) * nObj;
}

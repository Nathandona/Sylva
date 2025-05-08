#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in float aOcclusion;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 ourColor;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out float Occlusion;

void main() {
    // Calculate fragment position in world space for lighting
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Transform position to clip space
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    
    // Pass color and texture coordinates to fragment shader
    ourColor = aColor;
    TexCoord = aTexCoord;
    Occlusion = aOcclusion;
    
    // Calculate normal in world space - using normal matrix (transpose of inverse of model)
    // This protects normals from being incorrectly transformed by non-uniform scaling
    Normal = mat3(transpose(inverse(model))) * aNormal;
} 
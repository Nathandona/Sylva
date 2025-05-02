#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D textureSampler;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    // Set default light direction and color if not provided
    vec3 light = lightDir;
    if (length(lightDir) < 0.1) {
        light = normalize(vec3(0.5, 1.0, 0.3)); // Default light direction
    }
    
    vec3 lightCol = lightColor;
    if (length(lightColor) < 0.1) {
        lightCol = vec3(1.0, 1.0, 0.9); // Default to slightly warm white
    }
    
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightCol;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, light), 0.0);
    vec3 diffuse = diff * lightCol;
    
    // Combine lighting
    vec3 lighting = ambient + diffuse;
    
    // Sample texture
    vec4 texColor = texture(textureSampler, TexCoord);
    
    // If texture is missing (alpha is 0), use a debug color
    if (texColor.a < 0.1) {
        texColor = vec4(0.0, 1.0, 0.0, 1.0); // Bright green for debugging
    }
    
    // Apply lighting to texture color
    vec3 result = lighting * texColor.rgb;
    
    // Set final color
    FragColor = vec4(result, texColor.a);
} 
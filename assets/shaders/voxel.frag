#version 330 core
in vec3 ourColor;    // Base color from vertex attributes
in vec2 TexCoord;
in vec3 Normal;      // World-space normal
in vec3 FragPos;     // World-space fragment position
in float Occlusion;  // Received AO factor from vertex shader

out vec4 FragColor;

// Uniforms for lighting
uniform vec3 lightDir;            // Direction of the light (set from C++)
uniform vec3 uniformLightColor;   // Color of the directional light (set from C++)
uniform vec3 uniformAmbientColor; // Color of the ambient light (set from C++)
// uniform vec3 viewPos; // Already available from current shader if needed for specular, etc.
uniform float opacity = 1.0; // Optional opacity

void main() {
    // Normalize vectors
    vec3 norm = normalize(Normal);
    // Assuming lightDir is already normalized when set as a uniform
    vec3 normalizedLightDir = normalize(lightDir); 

    // Ambient lighting component, modulated by AO
    // Occlusion = 1.0 means no occlusion, 0.0 means full occlusion
    vec3 ambient = uniformAmbientColor * Occlusion;

    // Diffuse lighting component (can also be modulated by AO if desired, for softer shadows)
    float diff = max(dot(norm, normalizedLightDir), 0.0);
    vec3 diffuse = diff * uniformLightColor * Occlusion; // Modulating diffuse too for stronger AO effect

    // Combine lighting (ambient + diffuse)
    vec3 totalLight = ambient + diffuse;
    
    // Apply total lighting to the base voxel color (ourColor)
    vec3 result = totalLight * ourColor;
    
    // The existing edge darkening and saturation can be re-evaluated and added here later
    // For example:
    // float edgeFactor = pow(max(dot(norm, normalize(viewPos - FragPos)), 0.0), 1.5);
    // float edgeDarkening = mix(0.85, 1.0, edgeFactor);
    // result *= edgeDarkening;

    FragColor = vec4(result, opacity);
} 
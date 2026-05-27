#version 330 core
in vec3 ourColor;    // Base color from vertex attributes
in vec3 Normal;      // World-space normal
in vec3 FragPos;     // World-space fragment position
in float Occlusion;  // Received AO factor from vertex shader

out vec4 FragColor;

// Uniforms for lighting
uniform vec3 lightDir;            // Direction of the light (set from C++)
uniform vec3 uniformLightColor;   // Color of the directional light (set from C++)
uniform vec3 uniformAmbientColor; // Color of the ambient light (set from C++)
uniform vec3 viewPos;             // Camera world pos — also used for distance fog
uniform float opacity = 1.0;      // Optional opacity

// Distance fog — hides chunk pop-in at the view-distance edge and gives the
// world an atmospheric depth cue. Linear ramp from fogStart to fogEnd in
// world units; beyond fogEnd everything is fogColor.
uniform vec3 fogColor = vec3(0.5, 0.7, 0.9);
uniform float fogStart = 12.0;
uniform float fogEnd = 22.0;

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

    // Distance fog mix. Match the sky/clear color in C++ so fog blends
    // seamlessly with the background — chunks at the horizon fade away
    // instead of popping out of existence at the view-distance edge.
    float dist = length(viewPos - FragPos);
    float fogFactor = clamp((dist - fogStart) / max(fogEnd - fogStart, 0.0001), 0.0, 1.0);
    result = mix(result, fogColor, fogFactor);

    FragColor = vec4(result, opacity);
}
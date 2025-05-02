#version 330 core
in vec3 ourNormal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture0;
uniform bool hasTexture;

void main()
{
    if(hasTexture) {
        FragColor = texture(texture0, TexCoord);
    } else {
        // Simple color based on normal direction
        vec3 color = normalize(ourNormal) * 0.5 + 0.5;
        FragColor = vec4(color, 1.0);
    }
} 
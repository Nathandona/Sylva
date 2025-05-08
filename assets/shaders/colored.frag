#version 330 core
in vec3 ourColor;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec4 color;

void main() {
    // Use the vertex color directly
    FragColor = vec4(ourColor, 1.0);
} 
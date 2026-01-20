#version 460 core

in vec2 vUV;
out vec4 FragColor;

uniform vec3 topColor;
uniform vec3 bottomColor;

void main() {
    // Vertical gradient from bottom to top
    vec3 color = mix(bottomColor, topColor, vUV.y);
    FragColor = vec4(color, 1.0);
}

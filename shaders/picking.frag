#version 460 core

out vec4 FragColor;

uniform uint objectID;

void main() {
    // Encode object ID as color (supports up to 16 million objects)
    uint r = (objectID & 0x000000FFu);
    uint g = (objectID & 0x0000FF00u) >> 8u;
    uint b = (objectID & 0x00FF0000u) >> 16u;

    FragColor = vec4(float(r) / 255.0, float(g) / 255.0, float(b) / 255.0, 1.0);
}

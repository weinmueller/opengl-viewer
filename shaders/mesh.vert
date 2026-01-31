#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aSolutionValue;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out float SolutionValue;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalMatrix * aNormal;
    TexCoord = aTexCoord;
    SolutionValue = aSolutionValue;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}

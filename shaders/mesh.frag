#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

struct Light {
    vec3 direction;
    vec3 color;
    float ambient;
    float diffuse;
    float specular;
};

uniform Light light;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform float rimStrength;
uniform vec3 rimColor;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Ambient
    vec3 ambient = light.ambient * light.color;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * light.color;

    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = light.specular * spec * light.color;

    // Rim/Fresnel lighting - highlights edges facing away from viewer
    float rim = 1.0 - max(dot(viewDir, norm), 0.0);
    rim = smoothstep(0.4, 1.0, rim);  // Sharpen the rim
    vec3 rimLight = rimStrength * rim * rimColor;

    vec3 result = (ambient + diffuse + specular) * objectColor + rimLight;
    FragColor = vec4(result, 1.0);
}

#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float SolutionValue;

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
uniform sampler2D diffuseMap;
uniform bool hasTexture;

// Solution visualization uniforms
uniform bool showSolution;
uniform float solutionMin;
uniform float solutionMax;

// Blue-white-red colormap for solution visualization
vec3 solutionColormap(float value) {
    float t = clamp((value - solutionMin) / (solutionMax - solutionMin), 0.0, 1.0);

    // Blue at t=0, white at t=0.5, red at t=1
    vec3 blue = vec3(0.2, 0.4, 1.0);
    vec3 white = vec3(1.0, 1.0, 1.0);
    vec3 red = vec3(1.0, 0.3, 0.2);

    if (t < 0.5) {
        return mix(blue, white, t * 2.0);
    } else {
        return mix(white, red, (t - 0.5) * 2.0);
    }
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Get base color based on mode
    vec3 baseColor;
    if (showSolution) {
        baseColor = solutionColormap(SolutionValue);
    } else if (hasTexture) {
        baseColor = texture(diffuseMap, TexCoord).rgb;
    } else {
        baseColor = objectColor;
    }

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

    vec3 result = (ambient + diffuse + specular) * baseColor + rimLight;
    FragColor = vec4(result, 1.0);
}

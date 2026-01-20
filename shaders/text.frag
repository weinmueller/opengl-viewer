#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D fontTexture;
uniform vec4 textColor;
uniform vec4 bgColor;

void main() {
    float alpha = texture(fontTexture, TexCoord).r;

    // Mix background and text color based on font alpha
    FragColor = mix(bgColor, textColor, alpha);
}

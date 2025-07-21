#version 440 core

uniform sampler2D normTex;
in vec2 texCoords;
out vec4 FragColor;

void main() {
    FragColor = texture(normTex, texCoords);
}

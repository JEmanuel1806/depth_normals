#version 440 core
layout (location = 0) in vec2 inPos;  // [-1..1]
layout (location = 1) in vec2 inUV;   // [0..1]
out vec2 TexCoords;
void main() {
    TexCoords = inUV;
    gl_Position = vec4(inPos, 0.0, 1.0);
}
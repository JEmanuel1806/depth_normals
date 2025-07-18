#version 440 core

in vec3 fNormal;  
out vec4 FragColor;

void main() {
    vec3 color = normalize(fNormal) * 0.5 + 0.5; 
    FragColor = vec4(color, 1.0);
}
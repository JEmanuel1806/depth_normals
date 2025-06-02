#version 440 core
uniform isampler2D idTex;
uniform vec2 iResolution;

in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

void main() {
    ivec2 fragCoord = ivec2(gl_FragCoord.xy);
    int id = texelFetch(idTex, fragCoord, 0).r;

    if (id == -1) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); // black for empty
    } else {
        float hue = float(id % 256) / 255.0;
        FragColor = vec4(hue, hue, hue, 1.0); // grayscale ID
    }
}

#version 440 core

uniform isampler2D normTex;
in vec2 texCoords;
out vec4 FragColor;

void main()
{
    int id = texture(normTex, texCoords).r;

    if (id == -1) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);  // Black for background
    } else {
        // Normalize ID to a visible color range
        float color = float(id % 256) / 255.0;
        FragColor = vec4(color, color, color, 1.0);
    }
}

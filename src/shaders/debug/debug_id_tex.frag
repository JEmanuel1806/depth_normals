#version 440 core

uniform isampler2D idTex;   // deine ID-Textur
in vec2 texCoords;
out vec4 FragColor;

void main() {
    int id = texture(idTex, texCoords).r;

    if (id < 0) {
        
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);  
    } else {
        // ID sichtbar machen einfache Farb-Mappung
        float f = float(id % 256) / 255.0; 
        FragColor = vec4(f, f, f, 1.0);    
    }
}
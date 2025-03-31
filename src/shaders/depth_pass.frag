#version 440 core

layout(location = 0) out float FragDepth; 
layout(location = 1) out int IDOut;       

flat in int vertex_id;

void main() {
    FragDepth = gl_FragCoord.z;
    IDOut = vertex_id; // save the unique ID for this fragment
}


/* DEBUG
flat in int vertex_id;
out vec4 FragColor;

void main() {
    float hue = float(vertex_id) / 2111.0; // assuming 2111 points
    FragColor = vec4(hue, 1.0 - hue, fract(hue * 3.14), 1.0); // pseudo-rainbow
}

*/
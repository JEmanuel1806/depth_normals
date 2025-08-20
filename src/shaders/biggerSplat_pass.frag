#version 440 core

layout(location = 0) out float FragDepth;
layout(location = 1) out int IDOut;    

flat in int vertex_id;

void main() {
    IDOut = vertex_id; 
}


#version 440 core

layout(location = 0) out float FragDepth; 
layout(location = 1) out int IDOut;       

flat in int ID;

void main() {
    FragDepth = gl_FragCoord.z;
    IDOut = ID; // save the unique ID for this fragment
}
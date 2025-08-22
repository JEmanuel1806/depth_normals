#version 440 core

layout (location = 0) in int ID;
layout (location = 1) in vec3 position;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

uniform float pointSize;

flat out int vertex_id;

void main()
{
    vertex_id = ID;
    gl_Position = proj * view * model * vec4(position, 1.0);
    gl_PointSize = 30.0f;
}
#version 440 core

layout (location = 0) in vec3 position;
layout (location = 1) in int ID;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

flat out int vertex_id;

void main()
{
    vertex_id = ID;
    gl_Position = proj * view * model * vec4(position, 1.0);
    gl_PointSize = 14.0;
}
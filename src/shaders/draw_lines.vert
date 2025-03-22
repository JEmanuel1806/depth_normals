#version 440 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 vNormal;
out vec3 vPosition;

void main()
{
    vPosition = aPos;
    vNormal = aNormal;
}
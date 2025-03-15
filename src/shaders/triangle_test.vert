#version 440 core

layout (location = 0) in vec3 vertexPositions;

void main()
{
	gl_Position = vec4(vertexPositions.x, vertexPositions.y, vertexPositions.z, 1.0);
}